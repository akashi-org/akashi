#pragma once

#include <libakcore/memory.h>

// forward declaration
#ifndef PyObject_HEAD
struct _object;
typedef _object PyObject;
#endif

namespace akashi {
    namespace eval {

        struct KronArg;
        class PythonString;
        class PythonObject;
        class PythonFunc {
          public:
            explicit PythonFunc(core::borrowed_ptr<PythonObject> p_module, const char* func_name);
            virtual ~PythonFunc(void) noexcept;

            bool execute(const KronArg& arg);

            core::borrowed_ptr<PythonObject> get_func(void) const {
                return core::borrowed_ptr(m_func);
            }

          private:
            PyObject* compile_argument(int* args, int argc) const;

          private:
            core::owned_ptr<PythonObject> m_func;
            core::borrowed_ptr<PythonObject> m_pymodule;
            const char* m_func_name = nullptr;
            bool m_can_execute = false;
        };

    }
}
