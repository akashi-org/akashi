#pragma once

#include <libakcore/memory.h>
#include <libakcore/rational.h>
#include <libakcore/element.h>

#include <array>

namespace akashi {
    namespace buffer {
        class AVBuffer;
        class AVBufferData;
    }
    namespace state {
        class AKState;
    }
    namespace eval {
        struct GlobalContext;
    }
    namespace graphics {

        class FBO;
        class Camera;

        class OGLRenderContext final {
          public:
            explicit OGLRenderContext(core::borrowed_ptr<state::AKState> state,
                                      core::borrowed_ptr<buffer::AVBuffer> buffer);
            virtual ~OGLRenderContext();

            bool load_fbo(bool enable_alpha);

            const FBO& fbo() const;

            FBO& mut_fbo();

            core::borrowed_ptr<Camera> mut_camera();

            const core::borrowed_ptr<Camera> camera() const;

            core::Rational fps();

            std::array<int, 2> resolution();

            std::string default_font_path();

            int msaa();

            std::unique_ptr<buffer::AVBufferData> dequeue(std::string layer_uuid,
                                                          const core::Rational& pts);

            void use_default_blend_func() const;

            core::LayerContext get_base_layer(const core::PlaneContext& plane_ctx);

            std::vector<core::LayerContext> local_eval(const core::PlaneContext& plane_ctx);

          private:
            core::borrowed_ptr<state::AKState> m_state;
            core::borrowed_ptr<buffer::AVBuffer> m_buffer;

            core::owned_ptr<FBO> m_fbo;
            core::owned_ptr<Camera> m_camera;
        };

    }
}
