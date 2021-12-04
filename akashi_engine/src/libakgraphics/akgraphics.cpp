#include "./akgraphics.h"
#include "./item.h"
#include "./context.h"

#include "./backend/opengl.h"

#include <libakcore/memory.h>
#include <libakcore/element.h>
#include <libakcore/logger.h>
#include <libakstate/akstate.h>
#include <libakbuffer/avbuffer.h>

using namespace akashi::core;

namespace akashi {
    namespace graphics {

        AKGraphics::AKGraphics(core::borrowed_ptr<state::AKState> state,
                               core::borrowed_ptr<buffer::AVBuffer> buffer) {
            m_gfx_ctx = make_owned<OGLGraphicsContext>(state, buffer);
        }

        AKGraphics::~AKGraphics() {}

        bool AKGraphics::load_api(const GetProcAddress& get_proc_address,
                                  const EGLGetProcAddress& egl_get_proc_address) {
            return m_gfx_ctx->load_api(get_proc_address, egl_get_proc_address);
        }

        void AKGraphics::render(const RenderParams& params, const core::FrameContext& frame_ctx) {
            m_gfx_ctx->render(params, frame_ctx);
        }

        void AKGraphics::encode_render(EncodeRenderParams& params,
                                       const core::FrameContext& frame_ctx) {
            m_gfx_ctx->encode_render(params, frame_ctx);
        }

    }
}
