#pragma once

#include "../../core/module.h"
#include "../../core/class.h"

#include <libakcore/path.h>
#include <libakcore/memory.h>

#include <string_view>

namespace akashi {
    namespace eval {

        class PythonObject;
        class KronArg;

        namespace lib::akashi_core {

            class InitModule;
            class KronModule final : public PythonModule {
              public:
                explicit KronModule(PyObject* pymodule, core::borrowed_ptr<InitModule> init_module);
                virtual ~KronModule();

                core::owned_ptr<PythonInstance> KronArgs(const KronArg& arg);

              private:
                core::borrowed_ptr<InitModule> m_init_module;
                core::owned_ptr<PythonObject> m_kron_args_cls;
            };

        };

    }

}
