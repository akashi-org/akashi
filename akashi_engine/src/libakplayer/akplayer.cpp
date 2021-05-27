#include "./akplayer.h"
#include "./event.h"
#include "./eval_buffer.h"
#include "./loop/main_loop.h"
#include "./loop/decode_loop.h"
#include "./loop/watch_loop.h"

#include <libakbuffer/avbuffer.h>
#include <libakbuffer/video_queue.h>
#include <libakbuffer/audio_queue.h>
#include <libakcore/memory.h>
#include <libakcore/element.h>
#include <libakcore/rational.h>
#include <libakcore/logger.h>
#include <libakaudio/akaudio.h>
#include <libakgraphics/akgraphics.h>
#include <libakgraphics/item.h>
#include <libakstate/akstate.h>
#include <libakevent/akevent.h>

using namespace akashi::core;

namespace akashi {
    namespace player {

        AKPlayer::AKPlayer(core::borrowed_ptr<state::AKState> state) : m_state(state) {}

        AKPlayer::~AKPlayer() { m_audio->destroy(); }

        void AKPlayer::init(event::EventCallback cb, void* evt_ctx,
                            graphics::GetProcAddress get_proc_address,
                            graphics::EGLGetProcAddress egl_get_proc_address) {
            m_buffer = make_owned<buffer::AVBuffer>(m_state);

            m_eval_buf = make_owned<EvalBuffer>(m_state);

            m_event = make_owned<PlayerEvent>(cb, evt_ctx);
            m_event->run(
                PlayerEventContext{m_state, borrowed_ptr(m_eval_buf), borrowed_ptr(m_buffer)});

            m_watchloop = make_owned<WatchLoop>();
            WatchLoopContext wloop_ctx = {borrowed_ptr(m_event), borrowed_ptr(m_state)};
            m_watchloop->run(wloop_ctx);

            m_decoder = make_owned<DecodeLoop>();
            DecodeLoopContext dloop_ctx = {m_state, borrowed_ptr(m_event), borrowed_ptr(m_buffer)};
            m_decoder->run(dloop_ctx);

            m_audio = make_owned<audio::AKAudio>(
                m_state, borrowed_ptr(m_buffer),
                borrowed_ptr<event::AKEvent>(borrowed_ptr(m_event).operator->()));

            m_gfx = make_owned<graphics::AKGraphics>(m_state, borrowed_ptr(m_buffer));
            m_gfx->load_api(get_proc_address, egl_get_proc_address);

            m_mainloop = make_owned<MainLoop>();
            MainLoopContext mloop_ctx = {borrowed_ptr(this), m_state, borrowed_ptr(m_event),
                                         borrowed_ptr(m_eval_buf)};
            m_mainloop->run(mloop_ctx);
        }

        void AKPlayer::render(const graphics::RenderParams& params) {
            const auto current_frame_ctx = m_eval_buf->render_buf();

            if (to_rational(current_frame_ctx.pts) ==
                to_rational(EvalBuffer::BLANK_FRAME_CTX.pts)) {
                AKLOG_INFON("PlayerContext::render(): Cannot get eval buffer. Skipping rendering.");
                m_state->set_render_completed(true);
                return;
            }

            m_gfx->render(params, current_frame_ctx);

            bool need_first_render = false;
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                need_first_render = m_state->m_prop.need_first_render;
                if (m_state->m_prop.need_first_render) {
                    m_state->m_prop.need_first_render = false;
                }
            }
            if (need_first_render) {
                m_event->emit_seek_completed();
            }

            m_state->set_render_completed(true);
        }

        void AKPlayer::play() {
            AKLOG_INFON("Play play");
            m_state->set_play_ready(true);
            m_audio->play();
        }

        void AKPlayer::pause() {
            AKLOG_INFON("Play pause");
            m_state->set_play_ready(false, true);
            m_audio->pause();
        }

        void AKPlayer::seek(const core::Rational& seek_time) {
            bool on_seeking = false;
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                on_seeking = !m_state->get_seek_completed();
            }
            if (!on_seeking) {
                m_event->emit_seek(seek_time);
            }
        }

        void AKPlayer::relative_seek(const double ratio) {
            Rational seek_time;
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                Rational incr_time =
                    Rational(ratio) * to_rational(m_state->m_prop.render_prof.duration);
                Rational tpf = (Rational(1, 1) / m_state->m_prop.fps);
                auto real_incr_time = Rational((int64_t)((incr_time / tpf).to_decimal()), 1) * tpf;
                seek_time = m_state->m_prop.current_time + real_incr_time;
            }
            this->seek(seek_time);
        }

        void AKPlayer::frame_step(void) {
            Rational seek_time;
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                seek_time = m_state->m_prop.current_time + (Rational(1, 1) / m_state->m_prop.fps);
            }
            this->seek(seek_time);
        }

        void AKPlayer::frame_back_step(void) {
            Rational seek_time;
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                seek_time = m_state->m_prop.current_time - (Rational(1, 1) / m_state->m_prop.fps);
            }
            this->seek(seek_time);
        }

        void AKPlayer::set_render_prof(core::RenderProfile& render_prof) {
            bool trigger_reset_current_time = false;
            Rational current_time;
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                m_state->m_prop.render_prof = render_prof;

                // set play time to zero when the play time is longer than the updated duration
                if (m_state->m_prop.current_time >
                    to_rational(m_state->m_prop.render_prof.duration)) {
                    m_state->m_prop.current_time = Rational(0, 1);
                    m_state->m_prop.elapsed_time = Rational(0, 1);
                    trigger_reset_current_time = true;
                }
                current_time = m_state->m_prop.current_time;
            }

            if (trigger_reset_current_time) {
                m_event->emit_time_update(current_time);
            }

            m_gfx->load_fbo(render_prof);
        }

        void AKPlayer::inline_eval(const std::string& fpath, const std::string& elem_name) {
            m_event->emit_inline_eval(fpath, elem_name);
        }

        void AKPlayer::set_volume(const double volume) { m_state->m_atomic_state.volume = volume; }

        core::Rational AKPlayer::current_time() const { return m_audio->current_time(); }

        core::Rational AKPlayer::current_frame_time(void) {
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                return m_state->m_prop.current_time;
            }
        }

        bool AKPlayer::evalbuf_dequeue_ready() { return m_state->get_evalbuf_dequeue_ready(); }

    }
}
