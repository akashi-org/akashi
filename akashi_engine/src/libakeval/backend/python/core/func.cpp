#include "./func.h"

#include "../../../item.h"

#include "./string.h"
#include "./object.h"

#include "../utils/print.h"
#include "../parser.h"
#include "./value.h"
#include "./tuple.h"

#include <libakcore/logger.h>
#include <libakcore/memory.h>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

using namespace akashi::core;

namespace akashi {
    namespace eval {

        PythonFunc::PythonFunc(core::borrowed_ptr<PythonObject> p_module, const char* func_name)
            : m_pymodule(p_module) {
            m_func_name = func_name;
            m_func =
                make_owned<PythonObject>(PyObject_GetAttrString(p_module->get_raw(), m_func_name));

            m_can_execute = m_func && PyCallable_Check(m_func->get_raw());

            if (!m_can_execute) {
                if (PyErr_Occurred()) {
                    PyErr_Print();
                }
                AKLOG_ERROR("Cannot find function \"{}\"", m_func_name);
            }
        };

        PythonFunc::~PythonFunc(void) noexcept {};

        bool PythonFunc::execute(const KronArg& arg) {
            // if (!m_can_execute) {
            //     return false;
            // }

            auto Second = PythonObject(PyObject_GetAttrString(m_pymodule->get_raw(), "Second"));

            auto PyKronArgs =
                PythonObject(PyObject_GetAttrString(m_pymodule->get_raw(), "KronArgs"));

            auto sec =
                PythonObject(PyObject_CallObject(Second.get_raw(), make_tuple(1l, 1l)->get_raw()));

            auto comp_args =
                PythonObject(PyObject_CallObject(PyKronArgs.get_raw(),
                                                 make_tuple(sec.get_raw(), 30l)->get_raw()),
                             false);

            auto kron = PythonObject(PyObject_CallObject(m_func->get_raw(), nullptr));
            auto frame = make_owned<PythonObject>(
                PyObject_CallObject(kron.get_raw(), make_tuple(comp_args.get_raw())->get_raw()));

            parse_frameContext(borrowed_ptr(frame));

            // auto print_func = PyObject_GetAttrString(PyEval_GetBuiltins(), "print");
            // PyObject_CallFunctionObjArgs(print_func, kron);
            // PyObject_CallFunctionObjArgs(print_func, frame);

            // auto pArgs = this->compile_argument(&arg, 1);
            // if (pArgs == nullptr) {
            //     return false;
            // }

            // // auto pValue = PyObject_Call(m_func, pArgs, nullptr);
            // // Py_XDECREF(pArgs);

            // // if (PyErr_Occurred() == nullptr && pValue != nullptr) {
            // //     fprintf(stderr, "Result of call: %ld\n", PyLong_AsLong(pValue));
            // //     Py_DECREF(pValue);
            // // } else {
            // //     PyErr_Print();
            // //     fprintf(stderr, "Call failed\n");
            // //     Py_XDECREF(pValue);
            // //     return false;
            // // }

            // PyObject_Call(m_func, pArgs, nullptr);
            // Py_XDECREF(pArgs);

            // if (PyErr_Occurred() == nullptr) {
            //     fprintf(stderr, "success\n");
            // } else {
            //     PyErr_Print();
            //     fprintf(stderr, "Call failed\n");
            //     return false;
            // }

            return true;
        };

        PyObject* PythonFunc::compile_argument(int* args, int argc) const {
            auto pArgs = PyTuple_New(argc);
            for (int i = 0; i < argc; ++i) {
                auto pArgValue = PyLong_FromLong(args[i]);
                if (!pArgValue) {
                    Py_XDECREF(pArgs);
                    Py_XDECREF(pArgValue);
                    AKLOG_ERRORN("Cannot convert argument");
                    return nullptr;
                }
                /* pArgValue reference stolen here: */
                if (PyTuple_SetItem(pArgs, i, pArgValue) < 0) {
                    PyErr_Print();
                    Py_XDECREF(pArgs);
                    // Py_XDECREF(pArgValue);
                    return nullptr;
                }
            }

            // is it necessary to check the size?
            auto tuple_size = PyTuple_GET_SIZE(pArgs);
            if (argc != tuple_size) {
                AKLOG_ERROR(
                    "PythonFunc::compile_argument()  the size of pArgs({}) and the argument argc({}) does not match",
                    tuple_size, argc);
                Py_XDECREF(pArgs);
                return nullptr;
            }

            return pArgs;
        };

    }
}
