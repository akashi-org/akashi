#pragma once

// forward declaration
#ifndef PyObject_HEAD
struct _object;
typedef _object PyObject;
#endif

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

        namespace lib::akashi_core {
            class InitModule;
        }

        class PythonModule;
        struct KronArg;

        class PyEvalContext final : public EvalContext {
          public:
            explicit PyEvalContext(core::borrowed_ptr<state::AKState> state);
            virtual ~PyEvalContext(void) noexcept;

            void exit(void) override;

            core::FrameContext eval_kron(const char* module_path, const KronArg& arg) override;

            std::vector<core::FrameContext>
            eval_krons(const char* module_path, const core::Rational& start_time, const int fps,
                       const core::Rational& duration, const size_t length) override;

            core::RenderProfile render_prof(const char* module_path) override;

            void reload(const std::vector<watch::WatchEvent>& events) override;

            const std::vector<std::string> loaded_module_paths(bool kron_module_only) const;

            const std::vector<std::string> imported_module_paths(void);

            const state::EvalConfig& config(void);

          private:
            bool load_module(const core::Path& module_path, const core::Path& include_dir);

            void imported_module_each(const std::function<void(PyObject*)>& callback);

            void imported_inner_module_each(const std::function<void(const core::Path&)>& callback);

          private:
            std::unordered_map<std::string, PythonModule*> m_modules;
            core::borrowed_ptr<state::AKState> m_state;
            core::owned_ptr<lib::akashi_core::InitModule> m_corelib;
            bool m_exited = false;
        };

    }
}
