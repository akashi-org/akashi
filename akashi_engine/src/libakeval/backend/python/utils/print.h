#pragma once

#include <libakcore/memory.h>

#include <cstdio>

// forward declaration
#ifndef PyObject_HEAD
struct _object;
typedef _object PyObject;
#endif

namespace akashi {
    namespace eval {

        class PythonObject;

        bool py_print(PyObject* pyo, FILE* fp = stdout);

        bool py_print(core::borrowed_ptr<PythonObject> obj, FILE* fp = stdout);

    }
}
