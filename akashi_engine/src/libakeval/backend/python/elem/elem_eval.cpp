#include "./elem_eval.h"

#include "./elem.h"
#include "./elem_tracer.h"
#include "./elem_proxy.h"
#include "../../../item.h"

#include <libakcore/memory.h>
#include <libakcore/element.h>
#include <libakcore/logger.h>

#include <pybind11/embed.h>

namespace akashi {
    namespace eval {

        core::owned_ptr<GlobalContext> global_eval(const pybind11::object& elem,
                                                   const core::Rational& fps) {
            auto etype = elem_type(elem);
            // assert(etype == ElementType::ROOT); // accept only root for a while

            auto ctx = core::make_owned<GlobalContext>();
            ctx->interval = core::Rational(1l) / fps;
            ctx->duration = core::Rational(0l);
            assert(ctx->atom_proxies.empty());

            switch (etype) {
                case ElementType::ROOT: {
                    trace_root(elem, *ctx);
                    break;
                }
                case ElementType::SCENE: {
                    trace_scene(elem, *ctx);
                    break;
                }
                case ElementType::ATOM: {
                    trace_atom(elem, *ctx);
                    break;
                }
                case ElementType::LAYER: {
                    AKLOG_ERRORN("Currently Layer type is not supported");
                    break;
                }
                default: {
                    AKLOG_ERROR("Invalid Layer type found, {}", etype);
                    break;
                }
            }

            return ctx;
        }

        static int64_t find_proxy_index(const GlobalContext& ctx, const KronArg& arg) {
            // [TODO] impl faster way (binary search?)
            for (size_t i = 0; i < ctx.atom_proxies.size(); i++) {
                auto profile = ctx.atom_proxies[i]->computed_profile();
                if (to_rational(profile.from) <= arg.play_time &&
                    arg.play_time <= to_rational(profile.to)) {
                    return i;
                }
            }
            return -1;
        }

        core::FrameContext local_eval(const GlobalContext& ctx, const KronArg& arg) {
            core::FrameContext frame_ctx;
            frame_ctx.pts = arg.play_time.to_fraction();
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
