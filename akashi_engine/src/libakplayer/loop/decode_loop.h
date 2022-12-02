#pragma once

#include <libakcore/memory.h>
#include <libakcore/rational.h>
#include <libakcore/element.h>
#include <libakstate/akstate.h>

#include <thread>
#include <vector>
#include <string>
#include <atomic>

namespace akashi {
    namespace core {
        struct AtomProfile;
    }
    namespace buffer {
        class AVBuffer;
    }
    namespace state {
        class AKState;
    }
    namespace player {

        class PlayerEvent;

        class DecodeState final {
          public:
            explicit DecodeState(core::borrowed_ptr<state::AKState> state);
            virtual ~DecodeState() = default;

            void seek_update(void);

            void hr_update(void);

          public:
            core::Rational fps;
            core::Rational decode_pts;
            core::RenderProfile render_prof;
            size_t seek_id = 0;

          private:
            core::borrowed_ptr<state::AKState> m_state;
        };

        struct DecodeLoopContext {
            core::borrowed_ptr<state::AKState> state;
            core::borrowed_ptr<PlayerEvent> event;
            core::borrowed_ptr<buffer::AVBuffer> buffer;
        };

        class DecodeLoop final {
          public:
            explicit DecodeLoop(core::borrowed_ptr<state::AKState> state) : m_state(state) {}

            virtual ~DecodeLoop() = default;

            void close_and_wait(void) {
                if (m_th) {
                    m_is_alive.store(false);
                    m_state->set_evalbuf_dequeue_ready(true, true);
                    m_state->set_video_decode_ready(true, true);
                    m_state->set_audio_decode_ready(true, true);
                    m_state->set_seek_completed(true, true);
                    m_state->set_decode_layers_not_empty(true, true);
                    m_state->set_decode_loop_can_continue(true, true);

                    m_th->join();
                    delete m_th;
                    m_th = nullptr;
                }
            }

            void run(DecodeLoopContext ctx) {
                m_th = new std::thread(&DecodeLoop::decode_thread, ctx, this);
            };

          private:
            static void decode_thread(DecodeLoopContext ctx, DecodeLoop* loop);

            static bool seek_detected(core::borrowed_ptr<state::AKState> state,
                                      const DecodeState& decode_state);

            static bool hr_detected(core::borrowed_ptr<state::AKState> state,
                                    const DecodeState& decode_state);

          private:
            std::thread* m_th = nullptr;
            std::atomic<bool> m_is_alive = true;
            core::borrowed_ptr<state::AKState> m_state;
        };
    }
}
