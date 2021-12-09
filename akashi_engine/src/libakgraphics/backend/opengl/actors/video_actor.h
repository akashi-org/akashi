#pragma once

#include "./actor.h"

#include <libakcore/rational.h>
#include <libakcore/memory.h>

namespace akashi {
    namespace buffer {
        class AVBufferData;
    }
    namespace graphics {

        class OGLRenderContext;

        class VideoActor : public Actor {
            struct Pass;

          public:
            explicit VideoActor() = default;
            virtual ~VideoActor() = default;
            VideoActor(VideoActor&&) = default;

            bool create(const OGLRenderContext& ctx, const core::LayerContext& layer_ctx);

            bool render(OGLRenderContext& ctx, const core::Rational& pts) override;

            bool destroy(const OGLRenderContext& ctx) override;

          private:
            bool render_inner(OGLRenderContext& ctx, const core::Rational& pts);

            bool load_pass(const OGLRenderContext& ctx,
                           core::owned_ptr<buffer::AVBufferData>&& buf_data);

            bool load_shaders();

            void update_model_mat();

          private:
            VideoActor::Pass* m_pass = nullptr;
            core::Rational m_current_pts = core::Rational(-1, 1);
        };
    }

}
