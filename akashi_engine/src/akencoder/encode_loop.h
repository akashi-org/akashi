#pragma once

#include <libakcore/memory.h>
#include <thread>
#include <functional>
#include <mutex>
#include <atomic>

namespace akashi {
    namespace state {
        class AKState;
    }
    namespace encoder {

        struct EncodeLoopContext {
            core::borrowed_ptr<state::AKState> state;
        };

        struct ExitContext;
        class EncodeLoop final {
          public:
            explicit EncodeLoop(){};

            virtual ~EncodeLoop() = default;

            void close_and_wait() {
                if (m_th) {
                    m_should_close = true;
                    m_th->join();
                    delete m_th;
                    m_th = nullptr;
                }
            };

            void run(EncodeLoopContext ctx) {
                m_th = new std::thread(&EncodeLoop::encode_thread, ctx, this);
            };

            bool should_close() const { return m_should_close; }

          private:
            static void encode_thread(EncodeLoopContext ctx, EncodeLoop* loop);

          private:
            std::thread* m_th = nullptr;
            std::atomic<bool> m_should_close = false;
        };

    }
}
