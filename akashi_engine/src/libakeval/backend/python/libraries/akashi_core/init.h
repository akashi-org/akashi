#pragma once

#include "../../core/module.h"

#include <libakcore/path.h>
#include <libakcore/memory.h>

namespace akashi {
    namespace eval {

        namespace lib::akashi_core {

            class KronModule;
            class KronUtilsModule;
            class TimeModule;
            class InitModule final : public PythonModule {
              public:
                static core::owned_ptr<InitModule> from(void);

              public:
                explicit InitModule(core::owned_ptr<PythonModule> pymodule);
                virtual ~InitModule();

                core::borrowed_ptr<TimeModule> time(void) const {
                    return core::borrowed_ptr(m_time_module.get());
                }

                core::borrowed_ptr<KronModule> kron(void) const {
                    return core::borrowed_ptr(m_kron_module.get());
                }

                core::borrowed_ptr<KronUtilsModule> kron_utils(void) const {
                    return core::borrowed_ptr(m_kron_utils_module.get());
                }

              private:
                core::owned_ptr<PythonModule> m_init_module;
                core::owned_ptr<TimeModule> m_time_module;
                core::owned_ptr<KronModule> m_kron_module;
                core::owned_ptr<KronUtilsModule> m_kron_utils_module;
            };

        }

    }
}
