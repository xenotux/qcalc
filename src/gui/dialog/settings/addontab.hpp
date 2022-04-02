/**
 *  QCalc - Extensible programming calculator
 *  Copyright (C) 2021  Julian Zampiccoli
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef QCALC_ADDONTAB_HPP
#define QCALC_ADDONTAB_HPP

#include <QWidget>
#include <QPushButton>
#include <QListWidget>

#include "../../../addon/addonmetadata.hpp"

#include "../../widgets/addonitemwidget.hpp"

class AddonTab : public QWidget {
Q_OBJECT
signals:

    void addonEnableChanged(AddonItemWidget *item);

    void installPressed();

    void refreshPressed();

public slots:

    void setAddons(const std::map<std::string, bool> &addonState,
                   const std::map<std::string, AddonMetadata> &addonMetadata);

public:
    explicit AddonTab(QWidget *parent = nullptr);

private slots:
    void onAddonEnableChanged();

private:
    QPushButton *installButton;
    QPushButton *refreshButton;
    QListWidget *listWidget;
};

#endif //QCALC_ADDONTAB_HPP