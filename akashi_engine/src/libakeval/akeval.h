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

            core::RenderProfile render_prof(const std::string& module_path,
                                            const std::string& elem_name);

            void reload(const std::vector<watch::WatchEvent>& events);

          private:
            core::owned_ptr<EvalContext> m_eval_ctx;
            core::borrowed_ptr<state::AKState> m_state;
            std::thread::id m_tid;
            bool m_exited = false;
        };

    }

}
