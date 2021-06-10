#pragma once

#include "../../context.h"

#include <libakcore/memory.h>
#include <libakcore/rational.h>

#include <memory>
#include <array>
#include <vector>

namespace akashi {
    namespace core {
        struct RenderProfile;
        struct FrameContext;
    }
    namespace buffer {
        class AVBuffer;
        class AVBufferData;
    }
    namespace state {
        class AKState;
    }
    namespace graphics {

        struct GLRenderContext;
        struct GetProcAddress;
        struct EGLGetProcAddress;
        struct RenderParams;
        struct EncodeRenderParams;
        class QuadPass;
        class GLGraphicsContext : public GraphicsContext {
          public:
            explicit GLGraphicsContext(core::borrowed_ptr<state::AKState> state,
                                       core::borrowed_ptr<buffer::AVBuffer> buffer);
            virtual ~GLGraphicsContext();

            bool load_api(const GetProcAddress& get_proc_address,
                          const EGLGetProcAddress& egl_get_proc_address) override;

            bool load_fbo(const core::RenderProfile& render_prof, bool flip_y = true) override;

            void render(const RenderParams& params, const core::FrameContext& frame_ctx) override;

            void encode_render(EncodeRenderParams& params,
                               const core::FrameContext& frame_ctx) override;

            size_t loop_cnt();

            core::Rational fps();

            bool shader_reload();

            void set_shader_reload(bool reloaded);

            std::vector<const char*> updated_shader_paths(void);

            std::array<int, 2> resolution();

            std::unique_ptr<buffer::AVBufferData> dequeue(std::string layer_uuid,
                                                          const core::Rational& pts);

          private:
            core::borrowed_ptr<state::AKState> m_state;
            core::borrowed_ptr<buffer::AVBuffer> m_buffer;
            core::owned_ptr<GLRenderContext> m_render_ctx;
            QuadPass* m_fbo_pass = nullptr;
        };

    }
}
