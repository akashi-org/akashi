#pragma once

#include <libakcore/path.h>
#include <libakcore/memory.h>
#include <string_view>

// forward declaration
#ifndef PyObject_HEAD
struct _object;
typedef _object PyObject;
#endif

namespace akashi {
    namespace eval {

        class PythonObject;
        class PythonModule {
          public:
            explicit PythonModule(core::Path path) noexcept(false);
            explicit PythonModule(PyObject* pymodule) noexcept(false);
            virtual ~PythonModule(void) noexcept;

            bool reload(void);
            core::borrowed_ptr<PythonObject> get_module(void) const {
                return core::borrowed_ptr(m_module);
            }

          protected:
            core::Path m_path = core::Path("");
            core::owned_ptr<PythonObject> m_module;
        };

    }
}
