#include "./value.h"

#include "./object.h"
#include "./string.h"

#include <libakcore/class.h>
#include <libakcore/memory.h>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

using namespace akashi::core;

namespace akashi {
    namespace eval {

        /* integers */

        PythonValue PythonValue::from(long v, bool managed) {
            return PythonValue(PyLong_FromLong(v), managed);
        }

        PythonValue PythonValue::from(unsigned long v, bool managed) {
            return PythonValue(PyLong_FromUnsignedLong(v), managed);
        }

        PythonValue PythonValue::from(long long v, bool managed) {
            return PythonValue(PyLong_FromLongLong(v), managed);
        }

        PythonValue PythonValue::from(unsigned long long v, bool managed) {
            return PythonValue(PyLong_FromUnsignedLongLong(v), managed);
        }

        /* float */

        PythonValue PythonValue::from(double v, bool managed) {
            return PythonValue(PyFloat_FromDouble(v), managed);
        }

        /* string */

        PythonValue PythonValue::from(const char* v, bool managed) {
            return PythonValue(PyUnicode_FromString(v), managed);
        }

        /* bool */

        PythonValue PythonValue::from(bool v, bool managed) {
            return PythonValue(PyBool_FromLong(v), managed);
        }

        /* PyObject */

        PythonValue PythonValue::from(PyObject* pyo, bool managed) {
            return PythonValue(pyo, managed);
        }

        PythonValue::PythonValue(PyObject* pyo, bool managed) {
            m_value = core::make_owned<PythonObject>(pyo, managed);
        };

        template <typename T>
        T PythonValue::to(void) {
            // normally, unreachable
        }

        /* integers */

        template <>
        long PythonValue::to(void) {
            return PyLong_AsLong(m_value->get_raw());
        }

        template <>
        unsigned long PythonValue::to(void) {
            return PyLong_AsUnsignedLong(m_value->get_raw());
        }

        template <>
        long long PythonValue::to(void) {
            return PyLong_AsLongLong(m_value->get_raw());
        }

        template <>
        unsigned long long PythonValue::to(void) {
            return PyLong_AsUnsignedLongLong(m_value->get_raw());
        }

        /* float */

        template <>
        double PythonValue::to(void) {
            return PyFloat_AsDouble(m_value->get_raw());
        }

        /* string */

        template <>
        PythonString PythonValue::to(void) {
            return PythonString(m_value->get_raw());
        }

        /* bool */

        template <>
        bool PythonValue::to(void) {
            return PyLong_AsLong(m_value->get_raw()) == 1;
        }

    }
}
