#pragma once

#include <libakcore/memory.h>
#include <libakevent/akevent.h>

namespace akashi {

    namespace core {
        class Rational;
        struct RenderProfile;
        struct RenderContext;
    }
    namespace audio {
        class AKAudio;
    }
    namespace graphics {
        class AKGraphics;
        struct GetProcAddress;
        struct RenderParams;
    }
    namespace buffer {
        class AVBuffer;
    }
    namespace state {
        class AKState;
    }
    namespace player {

        class PlayerEvent;
        class EvalBuffer;
        class MainLoop;
        class DecodeLoop;
        class WatchLoop;
        class AKPlayer final {
          public:
            explicit AKPlayer(core::borrowed_ptr<state::AKState> state);
            virtual ~AKPlayer();

            void init(event::EventCallback cb, void* evt_ctx,
                      graphics::GetProcAddress get_proc_address);

            void render(const graphics::RenderParams& params);

            void play();

            void pause();

            void seek(const core::Rational& seek_time);

            void relative_seek(const core::Rational& rel_seek_time);

            void frame_step(void);

            void frame_back_step(void);

            void set_render_prof(core::RenderProfile& render_prof);

            // audio current time
            core::Rational current_time() const;

            bool evalbuf_dequeue_ready();

          private:
            core::borrowed_ptr<state::AKState> m_state;
            core::owned_ptr<PlayerEvent> m_event;
            core::owned_ptr<EvalBuffer> m_eval_buf;
            core::owned_ptr<buffer::AVBuffer> m_buffer;
            core::owned_ptr<audio::AKAudio> m_audio;
            core::owned_ptr<graphics::AKGraphics> m_gfx;

            core::owned_ptr<MainLoop> m_mainloop;
            core::owned_ptr<DecodeLoop> m_decoder;
            core::owned_ptr<WatchLoop> m_watchloop;
        };

    }
}
