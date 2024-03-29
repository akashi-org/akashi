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

            virtual void load(void) = 0;

            virtual bool loaded(void) const = 0;

            virtual void exit(void) = 0;

            virtual core::RenderProfile render_prof(const std::string& module_path,
                                                    const std::string& elem_name) = 0;

            virtual void reload(const std::vector<watch::WatchEvent>& events) = 0;
        };

    }

}
