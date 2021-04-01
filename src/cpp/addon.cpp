#include "addon.hpp"

#define PY_SSIZE_T_CLEAN

#include <Python.h>

#include <stdexcept>

#include "pyutil.hpp"

namespace Addon {
    void load(const std::string &moduleName) {
        PyObject *mod = PyImport_ImportModule(moduleName.c_str());
        if (mod == PyNull) {
            throw std::runtime_error(PyUtil::getError());
        }

        PyObject *dict = PyModule_GetDict(mod);
        if (dict == PyNull) {
            throw std::runtime_error(PyUtil::getError());
        }

        PyObject *key = PyUnicode_FromString("load");

        PyObject *function = PyDict_GetItem(dict, key);

        if (function != PyNull) {
            if (PyObject_CallNoArgs(function) == PyNull) {
                Py_DECREF(key);
                Py_DECREF(mod);
                throw std::runtime_error(PyUtil::getError());
            }
        } else {
            Py_DECREF(key);
            Py_DECREF(mod);
            throw std::runtime_error("load function not found in module");
        }

        Py_DECREF(key);
        Py_DECREF(mod);
    }

    void unload(const std::string &moduleName) {
        PyObject *mod = PyImport_ImportModule(moduleName.c_str());
        if (mod == PyNull) {
            throw std::runtime_error(PyUtil::getError());
        }

        PyObject *dict = PyModule_GetDict(mod);
        if (dict == PyNull) {
            throw std::runtime_error(PyUtil::getError());
        }

        PyObject *key = PyUnicode_FromString("unload");

        PyObject *function = PyDict_GetItem(dict, key);

        if (function != PyNull) {
            if (PyObject_CallNoArgs(function) == PyNull) {
                Py_DECREF(key);
                Py_DECREF(mod);
                throw std::runtime_error(PyUtil::getError());
            }
        } else {
            Py_DECREF(key);
            Py_DECREF(mod);
            throw std::runtime_error("unload function not found in module");
        }

        Py_DECREF(key);
        Py_DECREF(mod);
    }
}