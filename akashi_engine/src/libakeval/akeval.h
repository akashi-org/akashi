#pragma once

#include <libakcore/memory.h>
#include <libakcore/element.h>

#include <thread>
#include <vector>

namespace akashi {
    namespace core {
        class Rational;
        struct RenderProfile;
    }
    namespace state {
        class AKState;
    }
    namespace watch {
        struct WatchEvent;
    }
    namespace eval {

        struct KronArg;
        class EvalContext;
        class AKEval final {
          public:
            explicit AKEval(core::borrowed_ptr<state::AKState> state);
            virtual ~AKEval();

            void exit(void);

            core::FrameContext eval_kron(const char* module_path, const KronArg& kron_arg);

            std::vector<core::FrameContext>
            eval_krons(const char* module_path, const core::Rational& start_time, const int fps,
                       const core::Rational& duration, const size_t length);

            core::RenderProfile render_prof(const std::string& module_path,
                                            const std::string& elem_name);

            void reload(const std::vector<watch::WatchEvent>& events);

          private:
            core::owned_ptr<EvalContext> m_eval_ctx;
            std::thread::id m_tid;
        };

    }

}
