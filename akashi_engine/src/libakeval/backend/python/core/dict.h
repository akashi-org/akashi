#pragma once

#include "./object.h"

#define PY_SSIZE_T_CLEAN
#include <Python.h>

namespace akashi {
    namespace eval {

        inline size_t pydict_size(PyObject* pyo) { return PyDict_Size(pyo); }

        inline bool has_key(PyObject* pyo, const char* key) {
            auto key_pyo = PyUnicode_FromString(key);
            auto res = PyDict_Contains(pyo, key_pyo);
            Py_XDECREF(key_pyo);
            return res;
        }

        inline PythonObject get_item(PyObject* pyo, const char* key) {
            return PythonObject(PyDict_GetItemString(pyo, key), false);
        }

    }
}
