#include "./akgraphics.h"
#include "./item.h"
#include "./context.h"
#include "./backend/opengl.h"

#include <libakcore/memory.h>
#include <libakcore/element.h>
#include <libakcore/logger.h>
#include <libakaudio/akaudio.h>
#include <libakstate/akstate.h>
#include <libakbuffer/avbuffer.h>

using namespace akashi::core;

namespace akashi {
    namespace graphics {

        AKGraphics::AKGraphics(core::borrowed_ptr<state::AKState> state,
                               core::borrowed_ptr<buffer::AVBuffer> buffer,
                               core::borrowed_ptr<audio::AKAudio> audio) {
            m_gfx_ctx = make_owned<GLGraphicsContext>(state, buffer, audio);
        }

        AKGraphics::~AKGraphics() {}

        bool AKGraphics::load_api(const GetProcAddress& get_proc_address) {
            return m_gfx_ctx->load_api(get_proc_address);
        }

        bool AKGraphics::load_fbo(const core::RenderProfile& render_prof) {
            return m_gfx_ctx->load_fbo(render_prof);
        }

        void AKGraphics::render(const RenderParams& params, const core::FrameContext& frame_ctx) {
            m_gfx_ctx->render(params, frame_ctx);
        }

    }
}
