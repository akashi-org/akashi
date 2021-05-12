#pragma once

#include "../../context.h"

#include <libakcore/path.h>
#include <libakcore/memory.h>
#include <libakcore/element.h>

#include <unordered_map>
#include <string>
#include <vector>
#include <functional>

namespace akashi {
    namespace core {
        class Rational;
        struct RenderProfile;
    }
    namespace state {
        class AKState;
        struct EvalConfig;
    }
    namespace watch {
        struct WatchEvent;
    }
    namespace eval {

        struct KronArg;
        struct PyBind11Module;
        struct GlobalContext;

        class PyEvalContext final : public EvalContext {
          public:
            explicit PyEvalContext(core::borrowed_ptr<state::AKState> state);
            virtual ~PyEvalContext(void) noexcept;

            core::FrameContext eval_kron(const char* module_path, const KronArg& kron_arg) override;

            std::vector<core::FrameContext>
            eval_krons(const char* module_path, const core::Rational& start_time, const int fps,
                       const core::Rational& duration, const size_t length) override;

            core::RenderProfile render_prof(const std::string& module_path,
                                            const std::string& elem_name) override;

            void reload(const std::vector<watch::WatchEvent>& events) override;

            void exit(void) override;

          private:
            const state::EvalConfig& config(void);

            bool load_module(const core::Path& module_path, const core::Path& include_dir);

            bool register_deps_module(const core::Path& entry_path, const core::Path& include_dir);

          private:
            std::unordered_map<std::string, core::owned_ptr<PyBind11Module>> m_modules;
            core::borrowed_ptr<state::AKState> m_state;
            core::owned_ptr<GlobalContext> m_gctx;
            bool m_exited = false;
        };

    }
}
