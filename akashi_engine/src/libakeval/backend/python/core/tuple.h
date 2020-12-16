#pragma once

#include "./value.h"
#include "./object.h"

#include <libakcore/memory.h>
#include <libakcore/logger.h>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <type_traits>

// [TODO] make_tuple should be renamed b/c it has the same name as std::make_tuple

namespace akashi {
    namespace eval {

        inline bool set_tuple(PyObject*, size_t) { return true; };

        template <typename Head, typename... Tail>
        inline bool set_tuple(PyObject* tuple, size_t pos, const Head& v, const Tail&... tail) {
            auto item = PythonValue::from(v, false).value()->get_raw();
            if (!item) {
                Py_XDECREF(tuple);
                Py_XDECREF(item);
                AKLOG_ERRORN("set_tuple(): Cannot convert argument");
                return false;
            }

            /* item reference stolen here: */
            if (PyTuple_SetItem(tuple, pos, item) < 0) {
                PyErr_Print();
                Py_XDECREF(tuple);
                return false;
            }

            // [TODO] std::forward?
            return set_tuple(tuple, pos + 1, tail...);
        }

        // [TODO]
        // when args is PyObject*, this steals its reference, so be careful
        // For instance, when interacting with PythonObject, the object should be the one with
        // owned=false We should use types to handle this case.
        template <typename... Args>
        inline core::owned_ptr<PythonObject> make_tuple(const Args&... args) {
            auto tuple_size = sizeof...(Args);
            auto tuple = PyTuple_New(tuple_size);

            // [TODO] std::forward?
            if (!(set_tuple(tuple, 0, args...))) {
                Py_XDECREF(tuple);
                return nullptr;
            }

            // is it necessary to check the size?
            auto res_tuple_size = PyTuple_GET_SIZE(tuple);
            if (tuple_size != static_cast<unsigned long>(res_tuple_size)) {
                AKLOG_ERROR(
                    "make_tuple(): the size of pArgs({}) and the argument argc({}) does not match",
                    tuple_size, res_tuple_size);
                Py_XDECREF(tuple);
                return nullptr;
            }

            return core::make_owned<PythonObject>(tuple);
        };

    }
}
