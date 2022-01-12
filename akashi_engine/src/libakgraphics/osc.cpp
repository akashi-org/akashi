#include "./osc.h"
#include "./item.h"
#include "./context.h"

#include "./backend/opengl.h"

#include <libakcore/memory.h>
#include <libakcore/element.h>
#include <libakcore/logger.h>
#include <libakstate/akstate.h>

using namespace akashi::core;

namespace akashi {
    namespace graphics {

        OSCWidget::OSCWidget(core::borrowed_ptr<state::AKState> state) {
            m_osc_ctx = make_owned<OGLOSCContext>(state);
        }

        OSCWidget::~OSCWidget() {}

        bool OSCWidget::load_api(OSCEventCallback evt_cb, const RenderParams& params,
                                 const GetProcAddress& get_proc_address,
                                 const EGLGetProcAddress& egl_get_proc_address) {
            return m_osc_ctx->load_api(evt_cb, params, get_proc_address, egl_get_proc_address);
        }

        void OSCWidget::render(const RenderParams& params) { m_osc_ctx->render(params); }

        bool OSCWidget::emit_mouse_event(const OSCMouseEvent& event) {
            return m_osc_ctx->emit_mouse_event(event);
        }

        bool OSCWidget::emit_time_event(const OSCTimeEvent& event) {
            return m_osc_ctx->emit_time_event(event);
        }

    }
}
