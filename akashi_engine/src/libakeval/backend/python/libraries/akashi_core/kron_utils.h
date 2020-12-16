#pragma once

#include "../../core/module.h"
#include "../../core/class.h"

#include <libakcore/path.h>
#include <libakcore/memory.h>

#include <string_view>

namespace akashi {
    namespace core {
        class Rational;
        struct RenderProfile;
    }
    namespace eval {

        class PythonObject;
        class KronArg;

        namespace lib::akashi_core {

            class InitModule;
            class KronUtilsModule final : public PythonModule {
              public:
                explicit KronUtilsModule(PyObject* pymodule,
                                         core::borrowed_ptr<InitModule> init_module);
                virtual ~KronUtilsModule();

                core::owned_ptr<PythonObject> get_render_profile(core::owned_ptr<PythonObject> kron,
                                                                 const KronArg& arg);

                core::owned_ptr<PythonObject> get_frame_contexts(core::owned_ptr<PythonObject> kron,
                                                                 const core::Rational& start_time,
                                                                 const int fps,
                                                                 const core::Rational& duration,
                                                                 const size_t length);

              private:
                core::borrowed_ptr<InitModule> m_init_module;
                core::owned_ptr<PythonObject> m_get_render_profile_func;
                core::owned_ptr<PythonObject> m_get_frame_contexts_func;
            };

        };

    }

}
