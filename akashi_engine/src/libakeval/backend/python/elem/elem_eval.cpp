#include "./elem_eval.h"

#include "./elem.h"
#include "./elem_tracer.h"
#include "./elem_proxy.h"
#include "../../../item.h"

#include <libakcore/memory.h>
#include <libakcore/element.h>
#include <libakcore/logger.h>
#include <libakcore/uuid.h>

#include <pybind11/embed.h>

namespace akashi {
    namespace eval {

        core::owned_ptr<GlobalContext> global_eval(const pybind11::object& elem,
                                                   const core::Rational& fps) {
            auto ctx = core::make_owned<GlobalContext>();
            ctx->interval = core::Rational(1l) / fps;
            ctx->duration = core::Rational(0l);
            ctx->uuid = core::uuid();
            assert(ctx->atom_proxies.empty());

            trace_kron_context(elem, *ctx);

            return ctx;
        }

        static int64_t find_proxy_index(const GlobalContext& ctx, const KronArg& arg) {
            // [TODO] impl faster way (binary search?)
            for (size_t i = 0; i < ctx.atom_proxies.size(); i++) {
                auto profile = ctx.atom_proxies[i]->computed_profile();
                if (profile.from <= arg.play_time && arg.play_time <= profile.to) {
                    return i;
                }
            }
            return -1;
        }

        core::FrameContext local_eval(const GlobalContext& ctx, const KronArg& arg) {
            core::FrameContext frame_ctx;
            frame_ctx.pts = arg.play_time;
            auto proxy_idx = find_proxy_index(ctx, arg);
            if (proxy_idx < 0) {
                AKLOG_DEBUG("Could not find the suitable pts for: {}", arg.play_time.to_decimal());
                return frame_ctx;
            }
            frame_ctx.layer_ctxs = ctx.atom_proxies[proxy_idx]->eval(arg);
            return frame_ctx;
        }

    }
}
