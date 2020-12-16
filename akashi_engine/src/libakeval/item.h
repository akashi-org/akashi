#pragma once

#include <libakcore/path.h>
#include <libakcore/rational.h>

namespace akashi {
    namespace eval {

        struct KronArg {
            core::Rational play_time;
            long fps; // [TODO] double?
        };

    }

}
