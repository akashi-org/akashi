#pragma once

#include <libakcore/memory.h>
#include <libakcore/element.h>

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

        struct EvalConfig;
        struct KronArg;
        class EvalContext {
          public:
            explicit EvalContext(core::borrowed_ptr<state::AKState>){};
            virtual ~EvalContext(){};

            virtual void exit(void) = 0;

            virtual core::FrameContext eval_kron(const char* module_path,
                                                 const KronArg& kron_arg) = 0;

            virtual std::vector<core::FrameContext>
            eval_krons(const char* module_path, const core::Rational& start_time, const int fps,
                       const core::Rational& duration, const size_t length) = 0;

            virtual core::RenderProfile render_prof(const std::string& module_path,
                                                    const std::string& elem_name) = 0;
            // virtual bool evalbuf_dequeue_ready(const char* module_path) = 0;

            virtual void reload(const std::vector<watch::WatchEvent>& events) = 0;
        };

    }

}
