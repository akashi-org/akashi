#pragma once

#include <thread>
#include <functional>
#include <mutex>

namespace akashi {
    namespace ui {

        struct UILoopContext {
            int argc;
            char** argv;
        };

        class UILoop final {
          public:
            explicit UILoop(){};

            virtual ~UILoop() { this->terminate(); }

            void terminate() {
                {
                    std::lock_guard<std::mutex> lock(m_on_thread_exit.mtx);
                    if (m_on_thread_exit.func) {
                        m_on_thread_exit.func(m_on_thread_exit.ctx);
                    }
                    m_on_thread_exit.func = nullptr;
                }
                if (m_th) {
                    delete m_th;
                    m_th = nullptr;
                }
            };

            void run(UILoopContext ctx) {
                m_th = new std::thread(&UILoop::ui_thread, ctx, this);
                m_th->detach();
            };

            void set_on_thread_exit(std::function<void(void*)> on_thread_exit, void* ctx) {
                {
                    std::lock_guard<std::mutex> lock(m_on_thread_exit.mtx);
                    m_on_thread_exit.func = on_thread_exit;
                    m_on_thread_exit.ctx = ctx;
                }
                return;
            }

          private:
            static void ui_thread(UILoopContext ctx, UILoop* loop);

          private:
            std::thread* m_th = nullptr;
            struct {
                std::function<void(void*)> func;
                void* ctx;
                std::mutex mtx;
            } m_on_thread_exit;
        };

    }
}
