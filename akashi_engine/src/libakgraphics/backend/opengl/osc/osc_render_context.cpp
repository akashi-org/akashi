#include "./osc_render_context.h"
#include "./osc_camera.h"

#include "../../../item.h"
#include "../core/glc.h"

#include <libakcore/memory.h>
#include <libakcore/error.h>
#include <libakstate/akstate.h>

#include <libakcore/logger.h>

namespace akashi {
    namespace graphics {

        OSCRenderContext::OSCRenderContext(OSCEventCallback evt_cb, const RenderParams& params,
                                           core::borrowed_ptr<state::AKState> state)
            : m_evt_cb(evt_cb), m_state(state) {
            this->initialize_camera(params);
        }

        OSCRenderContext::~OSCRenderContext() {}

        void OSCRenderContext::resize(const RenderParams& params) {
            m_camera.reset(nullptr);
            this->initialize_camera(params);
        }

        void OSCRenderContext::emit_play_btn_clicked() {
            OSCInnerEvent event;
            event.name = OSCInnerEventName::PLAYBTN_CLICKED;
            m_evt_cb.cb(m_evt_cb.evt_ctx, event);
        }

        void OSCRenderContext::emit_volume_changed(double gain) {
            OSCInnerEvent event;
            event.name = OSCInnerEventName::VOLUME_CHANGED;
            event.args = std::shared_ptr<void>(new double(gain));
            m_evt_cb.cb(m_evt_cb.evt_ctx, event);
        }

        void OSCRenderContext::emit_update() {
            OSCInnerEvent event;
            event.name = OSCInnerEventName::UPDATE;
            m_evt_cb.cb(m_evt_cb.evt_ctx, event);
        }

        void OSCRenderContext::emit_frame_seek(int nframes) {
            OSCInnerEvent event;
            event.name = OSCInnerEventName::FRAME_SEEK;
            event.args = std::shared_ptr<void>(new int(nframes));
            m_evt_cb.cb(m_evt_cb.evt_ctx, event);
        }

        void OSCRenderContext::emit_seekbar_pressed(const core::Rational& seek_value) {
            OSCInnerEvent event;
            event.name = OSCInnerEventName::SEEKBAR_PRESSED;
            event.args = std::shared_ptr<void>(new core::Rational(seek_value));
            m_evt_cb.cb(m_evt_cb.evt_ctx, event);
        }

        void OSCRenderContext::emit_seekbar_moved(const core::Rational& seek_value) {
            OSCInnerEvent event;
            event.name = OSCInnerEventName::SEEKBAR_MOVED;
            event.args = std::shared_ptr<void>(new core::Rational(seek_value));
            m_evt_cb.cb(m_evt_cb.evt_ctx, event);
        }

        void OSCRenderContext::emit_seekbar_released(const core::Rational& seek_value) {
            OSCInnerEvent event;
            event.name = OSCInnerEventName::SEEKBAR_RELEASED;
            event.args = std::shared_ptr<void>(new core::Rational(seek_value));
            m_evt_cb.cb(m_evt_cb.evt_ctx, event);
        }

        core::borrowed_ptr<osc::Camera> OSCRenderContext::mut_camera() {
            return core::borrowed_ptr(m_camera.get());
        }

        const core::borrowed_ptr<osc::Camera> OSCRenderContext::camera() const {
            return core::borrowed_ptr(m_camera.get());
        }

        std::string OSCRenderContext::default_font_path() {
            std::string font_path;
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                font_path = m_state->m_prop.default_font_path;
            }
            return font_path;
        }

        void OSCRenderContext::use_default_blend_func() const {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        akashi::state::PlayState OSCRenderContext::play_state() {
            return m_state->m_atomic_state.icon_play_state;
        }

        core::Rational OSCRenderContext::duration() {
            core::Rational duration;
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                duration = m_state->m_prop.render_prof.duration;
            }
            return duration;
        }

        core::Rational OSCRenderContext::current_time() {
            core::Rational current_time;
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                current_time = m_state->m_prop.current_time;
            }
            return current_time;
        }

        core::Rational OSCRenderContext::fps() {
            core::Rational fps;
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                fps = m_state->m_prop.fps;
            }
            return fps;
        }

        double OSCRenderContext::gain() { return m_state->m_atomic_state.volume; }

        namespace priv {

            static osc::SeekBase get_seek_base(size_t level) {
                // assumes (vunit % nlabels) == (vunit % ruler_unit) == 0
                osc::SeekBase base{360, 40, 1}; // level0: aunit(360), 6sec(60fps), 15sec(24fps)
                if (level == 0) {
                    return base;
                } else {
                    base.sec_per_unit_coef *= 2 * level;
                    return base;
                }
            }
            constexpr const static size_t seek_max_level = 7200; // 24h(60fps)
        }

        osc::SeekBase OSCRenderContext::seek_base() const {
            // frame mode
            if (!m_is_second_mode) {
                return priv::get_seek_base(0);
            }
            // second mode
            else {
                return priv::get_seek_base(m_second_zoom_level);
            }
        }

        void OSCRenderContext::set_second_mode(bool second_mode) {
            m_is_second_mode = second_mode;
            if (second_mode) {
                m_zoom_level = m_second_zoom_level;
            } else {
                m_zoom_level = 0;
            }
        }

        void OSCRenderContext::update_second_seek_base() {
            auto sec_per_frame = core::Rational(1l) / this->fps();
            for (size_t i = 0; i < priv::seek_max_level - 1; i++) {
                auto cur_table = priv::get_seek_base(i);
                auto cur_band_dur =
                    core::Rational(cur_table.unit * cur_table.sec_per_unit_coef, 1) * sec_per_frame;
                if (this->duration() <= cur_band_dur) {
                    m_second_zoom_level = i;
                    return;
                }

                auto next_table = priv::get_seek_base(i + 1);
                auto next_band_dur =
                    core::Rational(next_table.unit * next_table.sec_per_unit_coef, 1) *
                    sec_per_frame;

                if ((this->duration() / next_band_dur) <= core::Rational(5, 4)) {
                    m_second_zoom_level = i;
                    return;
                }
            }

            m_second_zoom_level = priv::seek_max_level;
        }

        void OSCRenderContext::initialize_camera(const RenderParams& params) {
            int osc_width = params.screen_width;
            int osc_height = params.screen_height;

            osc::ProjectionState proj_state;
            proj_state.video_width = osc_width;
            proj_state.video_height = osc_height;

            osc::ViewState view_state;
            view_state.camera = glm::vec3(0, 0, 1);

            m_camera = core::make_owned<osc::Camera>(proj_state, &view_state);
        }

    }
}
