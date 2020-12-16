#pragma once

#include "../../context.h"

#include <libakcore/memory.h>

#include <memory>

namespace akashi {
    namespace core {
        struct RenderProfile;
        struct FrameContext;
        class Rational;
    }
    namespace buffer {
        class AVBuffer;
        class AVBufferData;
    }
    namespace audio {
        class AKAudio;
    }
    namespace state {
        class AKState;
    }
    namespace graphics {

        struct GLRenderContext;
        struct GetProcAddress;
        struct RenderParams;
        class GLGraphicsContext : public GraphicsContext {
          public:
            explicit GLGraphicsContext(core::borrowed_ptr<state::AKState> state,
                                       core::borrowed_ptr<buffer::AVBuffer> buffer,
                                       core::borrowed_ptr<audio::AKAudio> audio);
            virtual ~GLGraphicsContext();

            bool load_api(const GetProcAddress& get_proc_address) override;

            bool load_fbo(const core::RenderProfile& render_prof) override;

            void render(const RenderParams& params, const core::FrameContext& frame_ctx) override;

            size_t loop_cnt();

            core::Rational current_time() const;

            std::unique_ptr<buffer::AVBufferData> dequeue(std::string layer_uuid,
                                                          const core::Rational& pts);

          private:
            core::borrowed_ptr<state::AKState> m_state;
            core::borrowed_ptr<buffer::AVBuffer> m_buffer;
            core::borrowed_ptr<audio::AKAudio> m_audio;
            core::owned_ptr<GLRenderContext> m_render_ctx;
        };

    }
}
