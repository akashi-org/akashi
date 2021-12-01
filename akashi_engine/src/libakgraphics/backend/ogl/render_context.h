#pragma once

#include <libakcore/memory.h>
#include <libakcore/rational.h>

#include <array>

namespace akashi {
    namespace buffer {
        class AVBuffer;
        class AVBufferData;
    }
    namespace state {
        class AKState;
    }
    namespace graphics {

        class FBO;
        class Camera;

        class OGLRenderContext final {
          public:
            explicit OGLRenderContext(core::borrowed_ptr<state::AKState> state,
                                      core::borrowed_ptr<buffer::AVBuffer> buffer);
            virtual ~OGLRenderContext();

            bool load_fbo();

            const FBO& fbo() const;

            core::borrowed_ptr<Camera> mut_camera();

            const core::borrowed_ptr<Camera> camera() const;

            size_t loop_cnt();

            core::Rational fps();

            std::array<int, 2> resolution();

            std::unique_ptr<buffer::AVBufferData> dequeue(std::string layer_uuid,
                                                          const core::Rational& pts);

          private:
            core::borrowed_ptr<state::AKState> m_state;
            core::borrowed_ptr<buffer::AVBuffer> m_buffer;

            core::owned_ptr<FBO> m_fbo;
            core::owned_ptr<Camera> m_camera;
        };

    }
}
