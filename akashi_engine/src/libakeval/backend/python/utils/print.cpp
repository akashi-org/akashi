#include "./print.h"

#include "../core/object.h"

#include <libakcore/memory.h>
#include <libakcore/logger.h>

#include <cstdio>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

using namespace akashi::core;

namespace akashi {
    namespace eval {

        bool py_print(PyObject* pyo, FILE* fp) {
            int err_code = 0;
            if ((err_code = PyObject_Print(pyo, fp, Py_PRINT_RAW)) == -1) {
                AKLOG_ERROR("py_print(): PyObject_Print() failed, code:{}", err_code);
                return false;
            }
            if ((err_code = fflush(fp)) != 0) {
                AKLOG_ERROR("py_print(): fflush failed, code:{}", err_code);
                return false;
            }
            if (fp == stdout) {
                PySys_WriteStdout("\n");
            } else {
                PySys_WriteStderr("\n");
            }
            return true;
        }

        bool py_print(core::borrowed_ptr<PythonObject> obj, FILE* fp) {
            return py_print(obj->get_raw(), fp);
        }

    }
}
