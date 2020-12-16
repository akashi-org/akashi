#pragma once

#include <libakcore/memory.h>
#include <libakcore/rational.h>

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

            void loop_update(void);

          public:
            core::Rational fps;
            core::Rational decode_pts;
            std::string render_prof_uuid;
            std::vector<core::AtomProfile> atom_profiles;
            size_t loop_cnt = 0;
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
            explicit DecodeLoop(){};

            virtual ~DecodeLoop() {
                if (m_th) {
                    delete m_th;
                }
            }

            void destroy(void) { m_is_alive.store(false); }

            void run(DecodeLoopContext ctx) {
                m_th = new std::thread(&DecodeLoop::decode_thread, ctx, this);
                m_th->detach();
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
        };
    }
}
