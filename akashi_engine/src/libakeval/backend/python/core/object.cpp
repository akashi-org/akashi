#include "./object.h"

#include <libakcore/memory.h>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

using namespace akashi::core;

namespace akashi {
    namespace eval {

        PythonObject::~PythonObject(void) noexcept {
            // if (PyErr_Occurred() != nullptr) {
            //     PyErr_Print();
            // }
            if (m_pyo && m_owned) {
                Py_XDECREF(m_pyo);
            }
        }

        core::owned_ptr<PythonObject> PythonObject::attr(const char* attr_name) const {
            return make_owned<PythonObject>(PyObject_GetAttrString(m_pyo, attr_name));
        }

        bool PythonObject::has_attr(const char* attr_name) const {
            return PyObject_HasAttrString(m_pyo, attr_name);
        }

        bool PythonObject::is_none(void) const { return m_pyo == Py_None; }

    }
}
