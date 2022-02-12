#pragma once

#include "./elem_proxy.h"

#include <libakcore/memory.h>
#include <libakcore/rational.h>
#include <libakcore/element.h>

#include <vector>

namespace pybind11 {
    class object;
}

namespace akashi {
    namespace eval {

        struct KronArg;
        class AtomProxy;

        struct GlobalContext {
            std::vector<core::owned_ptr<AtomProxy>> atom_proxies;
            core::Rational sec_per_frame;
            core::Rational duration;
            std::string uuid;
        };

        core::owned_ptr<GlobalContext> global_eval(const pybind11::object& elem,
                                                   const core::Rational& fps);

        core::FrameContext local_eval(const GlobalContext& ctx, const KronArg& arg);

    }
}
