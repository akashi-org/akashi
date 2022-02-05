#pragma once

#include "../../../osc.h"

#include <libakcore/memory.h>
#include <libakcore/rational.h>
#include <libakstate/akstate.h>

namespace akashi {
    namespace state {
        class AKState;
    }
    namespace graphics::osc {
        class Camera;

        struct SeekBase {
            size_t unit = 360;
            size_t ruler_unit = 40;
            size_t sec_per_unit_coef = 1;
        };

    }
    namespace graphics {

        struct RenderParams;

        class OSCRenderContext final {
          public:
            explicit OSCRenderContext(OSCEventCallback evt_cb, const RenderParams& params,
                                      core::borrowed_ptr<state::AKState> state);
            virtual ~OSCRenderContext();

            void emit_play_btn_clicked();

            void emit_volume_changed(double gain);

            void emit_update();

            void emit_frame_seek(int nframes);

            void emit_seekbar_pressed(const core::Rational& seek_value);

            void emit_seekbar_moved(const core::Rational& seek_value);

            void emit_seekbar_released(const core::Rational& seek_value);

            core::borrowed_ptr<osc::Camera> mut_camera();

            const core::borrowed_ptr<osc::Camera> camera() const;

            std::string default_font_path();

            void use_default_blend_func() const;

            akashi::state::PlayState play_state();

            core::Rational duration();

            core::Rational current_time();

            core::Rational fps();

            double gain();

            osc::SeekBase seek_base() const;

            size_t zoom_level() const { return m_zoom_level; }

            void update_second_seek_base();

            void set_second_mode(bool second_mode);

            bool is_second_mode() const { return m_is_second_mode; }

          private:
            OSCEventCallback m_evt_cb;
            core::borrowed_ptr<state::AKState> m_state;
            core::owned_ptr<osc::Camera> m_camera;
            bool m_is_second_mode = true;
            size_t m_zoom_level = 1;
            size_t m_second_zoom_level = 1;
        };

    }
}
