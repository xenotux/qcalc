#include "gui/mainwindow.hpp"
#include "ui_mainwindow.h"

#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <QFileDialog>

#include "cpython/pyutil.hpp"
#include "addonmanager.hpp"
#include "addonhelper.hpp"
#include "paths.hpp"
#include "serializer.hpp"
#include "io.hpp"
#include "settingkeys.hpp"

#include "math/numberformat.hpp"
#include "math/expressionparser.hpp"

#include "gui/settingsdialog.hpp"
#include "gui/widgets/historywidget.hpp"
#include "gui/widgets/symbolseditor.hpp"

#include "cpython/modules/exprtkmodule.hpp"

#define ADDONS_FILE "/addons.json"
#define SETTINGS_FILE "/settings.json"

#define PRECISION_DEFAULT 9

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow()) {
    ui->setupUi(this);

    history = new HistoryWidget(this);
    history->setObjectName("widget_history");

    ui->tab_history->layout()->addWidget(history);

    symbolsEditor = new SymbolsEditor(this);
    symbolsEditor->setObjectName("widget_symtable_editor");

    connect(symbolsEditor, SIGNAL(onSymbolsChanged(const SymbolTable &)), this,
            SLOT(onSymbolTableChanged(const SymbolTable &)));

    ui->tab_symbols->layout()->addWidget(symbolsEditor);

    connect(ui->actionSettings, SIGNAL(triggered(bool)), this, SLOT(onActionSettings()));
    connect(ui->actionExit, SIGNAL(triggered(bool)), this, SLOT(onActionExit()));
    connect(ui->actionAbout, SIGNAL(triggered(bool)), this, SLOT(onActionAbout()));
    connect(ui->actionImport_Symbols, SIGNAL(triggered(bool)), this, SLOT(onActionImportSymbolTable()));
    connect(ui->actionExport_Symbols, SIGNAL(triggered(bool)), this, SLOT(onActionExportSymbolTable()));

    connect(ui->lineEdit_input, SIGNAL(returnPressed()), this, SLOT(onInputReturnPressed()));

    connect(this,
            SIGNAL(signalInputTextChange(const QString &)),
            ui->lineEdit_input,
            SLOT(setText(const QString &)));

    connect(this,
            SIGNAL(signalExpressionEvaluated(const QString &, const QString &)),
            history,
            SLOT(addContent(const QString &, const QString &)));

    std::string settingsFilePath = Paths::getAppDataDirectory().append(SETTINGS_FILE);
    if (QFile(settingsFilePath.c_str()).exists()) {
        try {
            settings = Serializer::deserializeSettings(IO::fileReadAllText(settingsFilePath));
        }
        catch (const std::runtime_error &e) {
            QMessageBox::warning(this, "Failed to load settings", e.what());
            settings = {};
        }
    }

    int precision = settings.value(SETTING_KEY_PRECISION, PRECISION_DEFAULT).toInt();

    mpfr::mpreal::set_default_prec(mpfr::digits2bits(precision));

    symbolsEditor->setSymbols(symbolTable);

    symbolsEditor->setConversionPrecision(precision);

    ExprtkModule::initialize();

    PyUtil::initializePython();
    PyUtil::addModuleDirectory(Paths::getSystemDirectory());
    PyUtil::addModuleDirectory(Paths::getAddonDirectory());

    std::string enabledAddonsFilePath = Paths::getAppDataDirectory().append(ADDONS_FILE);

    std::set<std::string> enabledAddons;
    if (QFile(enabledAddonsFilePath.c_str()).exists()) {
        try {
            enabledAddons = Serializer::deserializeSet(IO::fileReadAllText(enabledAddonsFilePath));
        }
        catch (const std::runtime_error &e) {
            QMessageBox::warning(this, "Failed to load enabled addons", e.what());
            enabledAddons.clear();
        }
    }

    //Check for enabled addons which dont exist anymore.
    std::set<std::string> availableAddons;
    auto addons = AddonHelper::getAvailableAddons(Paths::getAddonDirectory());
    for (auto &addon : enabledAddons) {
        if (addons.find(addon) != addons.end())
            availableAddons.insert(addon);
    }

    AddonManager::setActiveAddons(availableAddons, *this);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event) {
    exitRoutine();
}

void MainWindow::resizeEvent(QResizeEvent *event) {
}

void MainWindow::onAddonLoadFail(const std::string &moduleName, const std::string &error) {
    QMessageBox::warning(this, "Failed to load module",
                         ("Module " + moduleName + " failed to load, Error: " + error).c_str());
}

void MainWindow::onAddonUnloadFail(const std::string &moduleName, const std::string &error) {
    QMessageBox::warning(this, "Failed to unload module",
                         ("Module " + moduleName + " failed to unload, Error: " + error).c_str());
}

void MainWindow::onInputReturnPressed() {
    try {
        std::string inputText = ui->lineEdit_input->text().toStdString();
        ArithmeticType value = ExpressionParser::evaluate(inputText, symbolTable);

        symbolsEditor->setSymbols(symbolTable);

        std::string resultText = NumberFormat::toDecimal(value, settings.value(SETTING_KEY_PRECISION,
                                                                               PRECISION_DEFAULT).toInt());

        emit signalExpressionEvaluated(inputText.c_str(), resultText.c_str());

        inputText = resultText;

        emit signalInputTextChange(inputText.c_str());
    }
    catch (const std::runtime_error &e) {
        QMessageBox::warning(this, "Failed to evaluate expression", e.what());
    }
}

void MainWindow::onSymbolTableChanged(const SymbolTable &symbolTableArg) {
    this->symbolTable = symbolTableArg;
    symbolsEditor->setSymbols(symbolTable);
}

void MainWindow::onActionSettings() {
    SettingsDialog dialog;
    dialog.setPrecision(settings.value(SETTING_KEY_PRECISION, PRECISION_DEFAULT).toInt());
    dialog.setEnabledAddons(AddonManager::getActiveAddons());
    dialog.show();
    if (dialog.exec() == QDialog::Accepted) {
        int precision = dialog.getPrecision();
        settings.setValue(SETTING_KEY_PRECISION, precision);

        mpfr::mpreal::set_default_prec(mpfr::digits2bits(precision));

        symbolsEditor->setConversionPrecision(precision);

        //Set the precision of all symbol table variables and constants (In the future this should be more fine grained eg. different precision for every symbol)
        auto copy = symbolTable.getVariables();
        for (auto &p : copy) {
            p.second.setPrecision(mpfr::digits2bits(precision));
            symbolTable.remove(p.first);
            symbolTable.setVariable(p.first, p.second);
        }
        copy = symbolTable.getConstants();
        for (auto &p : copy) {
            p.second.setPrecision(mpfr::digits2bits(precision));
            symbolTable.remove(p.first);
            symbolTable.setConstant(p.first, p.second);
        }

        symbolsEditor->setSymbols(symbolTable);

        std::set<std::string> addons = dialog.getEnabledAddons();

        AddonManager::setActiveAddons(addons, *this);

        try {
            std::string dataDir = Paths::getAppDataDirectory();

            if (!QDir(dataDir.c_str()).exists())
                QDir().mkpath(dataDir.c_str());

            IO::fileWriteAllText(dataDir.append(ADDONS_FILE), Serializer::serializeSet(addons));
        }
        catch (const std::runtime_error &e) {
            QMessageBox::warning(this, "Failed to save enabled addons", e.what());
        }
    }
}

void MainWindow::onActionExit() {
    exitRoutine();
    QCoreApplication::quit();
}

void MainWindow::onActionAbout() {
    QMessageBox::about(this, "About " + QApplication::applicationDisplayName(),
                       QApplication::applicationDisplayName()
                       + " "
                       + QApplication::applicationVersion()
                       + "\n\nCopyright (c) 2021 Julian Zampiccoli"
                       + "\n\n"
                       + QString(R"LLL(Source:
    https://github.com/xenotux/qcalc

Powered by:
    https://github.com/ArashPartow/exprtk
    https://github.com/python/cpython
    https://github.com/qt
)LLL"));
}

void MainWindow::onActionImportSymbolTable() {
    QFileDialog dialog(this);
    dialog.setWindowTitle("Import Symbols...");
    dialog.setFileMode(QFileDialog::ExistingFile);

    if (!dialog.exec()) {
        return;
    }

    QStringList list = dialog.selectedFiles();

    if (list.size() != 1) {
        return;
    }

    std::string filepath = list[0].toStdString();

    std::set<std::string> addons = AddonManager::getActiveAddons();
    AddonManager::setActiveAddons({}, *this);

    try {
        symbolTable = Serializer::deserializeTable(IO::fileReadAllText(filepath));
        symbolsEditor->setSymbols(symbolTable);
        QMessageBox::information(this, "Import successful", ("Successfully imported symbols from " + filepath).c_str());
    }
    catch (const std::exception &e) {
        std::string error = "Failed to import symbols from ";
        error += filepath;
        error += " Error: ";
        error += e.what();
        QMessageBox::warning(this, "Import failed", error.c_str());
    }

    AddonManager::setActiveAddons(addons, *this);
}

void MainWindow::onActionExportSymbolTable() {
    QFileDialog dialog(this);
    dialog.setWindowTitle("Import Symbols...");
    dialog.setFileMode(QFileDialog::AnyFile);

    if (!dialog.exec()) {
        return;
    }

    QStringList list = dialog.selectedFiles();

    if (list.size() != 1) {
        return;
    }

    std::string filepath = list[0].toStdString();

    try {
        IO::fileWriteAllText(filepath, Serializer::serializeTable(symbolTable));
        QMessageBox::information(this,
                                 "Export successful",
                                 ("Successfully exported symbols to " + filepath).c_str());
    }
    catch (const std::exception &e) {
        std::string error = "Failed to export symbols to ";
        error += filepath;
        error += " Error: ";
        error += e.what();
        QMessageBox::warning(this, "Export failed", error.c_str());
    }
}

void MainWindow::exitRoutine() {
    AddonManager::setActiveAddons({}, *this);

    try {
        std::string dataDir = Paths::getAppDataDirectory();

        if (!QDir(dataDir.c_str()).exists())
            QDir().mkpath(dataDir.c_str());

        IO::fileWriteAllText(dataDir.append(SETTINGS_FILE),
                             Serializer::serializeSettings(settings));
    }
    catch (const std::exception &e) {
        QMessageBox::warning(this, "Failed to save settings", e.what());
    }
}
