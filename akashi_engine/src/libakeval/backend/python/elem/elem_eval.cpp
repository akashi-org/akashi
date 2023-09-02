#include "./elem_eval.h"

#include "./elem.h"
#include "./elem_tracer.h"
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
            ctx->sec_per_frame = core::Rational(1l) / fps;
            ctx->duration = core::Rational(0l);
            ctx->uuid = core::uuid();
            assert(ctx->atom_proxies.empty());

            trace_kron_context(elem, *ctx);

            ctx->local_eval = [](core::borrowed_ptr<GlobalContext> gctx, const KronArg& arg) {
                core::FrameContext frame_ctx;
                frame_ctx.pts = arg.play_time;

                // [XXX] for multiple atoms
                // int64_t atom_idx = -1;
                // for (size_t i = 0; i < gctx->atom_proxies.size(); i++) {
                //     auto profile = gctx->atom_proxies[i].computed_profile(*gctx);
                //     if (profile.from <= arg.play_time && arg.play_time <= profile.to) {
                //         atom_idx = i;
                //     }
                // }
                // if (atom_idx < 0) {
                //     AKLOG_DEBUG("Could not find the suitable pts for: {}",
                //                 arg.play_time.to_decimal());
                //     return frame_ctx;
                // }
                int64_t atom_idx = 0;

                frame_ctx.plane_ctxs = gctx->atom_proxies[atom_idx].eval(gctx, arg);
                frame_ctx.atom_static_profile = gctx->atom_proxies[atom_idx].static_profile();
                return frame_ctx;
            };

            return ctx;
        }

    }
}
