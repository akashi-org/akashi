#pragma once

#include "./value.h"
#include "./object.h"

#include <libakcore/memory.h>
#include <libakcore/logger.h>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <vector>
#include <functional>

namespace akashi {
    namespace eval {

        inline bool set_list(PyObject*, size_t) { return true; };

        template <typename Head, typename... Tail>
        inline bool set_list(PyObject* list, size_t pos, const Head& v, const Tail&... tail) {
            auto item = PythonValue::from(v, false).value()->get_raw();
            if (!item) {
                Py_XDECREF(list);
                Py_XDECREF(item);
                AKLOG_ERRORN("set_list(): Cannot convert argument");
                return false;
            }

            /* item reference stolen here: */
            if (PyList_SetItem(list, pos, item) < 0) {
                PyErr_Print();
                Py_XDECREF(list);
                return false;
            }

            // [TODO] std::forward?
            return set_list(list, pos + 1, tail...);
        }

        // [TODO]
        // when args is PyObject*, this steals its reference, so be careful
        // For instance, when interacting with PythonObject, the object should be the one with
        // owned=false We should use types to handle this case.
        template <typename... Args>
        inline core::owned_ptr<PythonObject> make_list(const Args&... args) {
            auto list_size = sizeof...(Args);
            auto list = PyList_New(list_size);

            // [TODO] std::forward?
            if (!(set_list(list, 0, args...))) {
                Py_XDECREF(list);
                return nullptr;
            }

            // is it necessary to check the size?
            auto res_list_size = PyList_GET_SIZE(list);
            if (list_size != static_cast<unsigned long>(res_list_size)) {
                AKLOG_ERROR(
                    "make_list(): the size of pArgs({}) and the argument argc({}) does not match",
                    list_size, res_list_size);
                Py_XDECREF(list);
                return nullptr;
            }

            return core::make_owned<PythonObject>(list);
        };

        template <typename ItemType>
        inline bool set_list_from_vec(PyObject* list, size_t pos,
                                      const std::vector<ItemType>& items) {
            if (pos >= items.size()) {
                return true;
            }

            auto item = PythonValue::from(items[pos], false).value()->get_raw();
            if (!item) {
                Py_XDECREF(list);
                Py_XDECREF(item);
                AKLOG_ERRORN("set_list(): Cannot convert argument");
                return false;
            }

            /* item reference stolen here: */
            if (PyList_SetItem(list, pos, item) < 0) {
                PyErr_Print();
                Py_XDECREF(list);
                return false;
            }

            // [TODO] std::forward?
            return set_list_from_vec(list, pos + 1, items);
        }

        template <typename ItemType>
        inline core::owned_ptr<PythonObject> make_list(const std::vector<ItemType>& items) {
            auto list_size = items.size();
            auto list = PyList_New(list_size);

            if (!(set_list_from_vec(list, 0, items))) {
                Py_XDECREF(list);
                return nullptr;
            }

            // is it necessary to check the size?
            auto res_list_size = PyList_GET_SIZE(list);
            if (list_size != static_cast<unsigned long>(res_list_size)) {
                AKLOG_ERROR(
                    "make_list(): the size of pArgs({}) and the argument argc({}) does not match",
                    list_size, res_list_size);
                Py_XDECREF(list);
                return nullptr;
            }

            return core::make_owned<PythonObject>(list);
        }

        // [XXX] Be careful that all of the callback arguments are Borrowed reference and their
        // lifetimes are temporary
        template <typename ItemType>
        inline std::vector<ItemType>
        map_list(PyObject* list,
                 const std::function<ItemType(core::borrowed_ptr<const PythonObject>)>& cb) {
            auto list_size = PyList_Size(list);
            std::vector<ItemType> res_vec;
            for (int i = 0; i < list_size; i++) {
                auto item = PyList_GetItem(list, i);
                if (!item) {
                    PyErr_Print();
                    return {};
                } else {
                    const auto item_pyo = PythonObject(item, false);
                    res_vec.push_back(cb(core::borrowed_ptr(&item_pyo)));
                }
            }
            return res_vec;
        }

    }
}
