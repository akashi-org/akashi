#include "./string.h"

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <stdexcept>

namespace akashi {
    namespace eval {

        PythonString::PythonString(PyObject* str, const char* encoding, const char* errors) {
            if (!str) {
                // includes the case where the string is blank
                m_bytes_obj = nullptr;
                return;
            }

            auto bytes_obj = PyUnicode_AsEncodedString(str, encoding, errors);
            if (!bytes_obj) {
                PyErr_Print();
                throw std::runtime_error("PythonString::PythonString() Failed to load PyBytes");
            }
            m_bytes_obj = bytes_obj;
        }

        PythonString::~PythonString(void) noexcept { Py_XDECREF(m_bytes_obj); }

        const char* PythonString::char_p(void) const {
            if (!m_bytes_obj) {
                return "";
            }
            auto cstr = PyBytes_AsString(m_bytes_obj);
            if (!cstr) {
                PyErr_Print();
                return nullptr;
            }
            return cstr;
        }

    }
}
