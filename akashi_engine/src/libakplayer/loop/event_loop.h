#pragma once

#include <libakcore/memory.h>

#include <functional>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace akashi {
    namespace core {
        class Rational;
    }
    namespace eval {
        class AKEval;
    }
    namespace buffer {
        class AVBuffer;
    }
    namespace watch {
        struct WatchEventList;
    }
    namespace state {
        class AKState;
    }
    namespace player {

        class PlayerEvent;
        class EvalBuffer;

        class SeekManager;
        class HRManager;

        enum class InnerEventName {
            DUMMY = -1,
            PULL_EVAL_BUFFER = 0,
            PULL_RENDER_PROFILE,
            SEEK,
            HOT_RELOAD,
        };

        struct InnerEvent {
            InnerEventName name = InnerEventName::DUMMY;
            void* ctx = nullptr;
        };

        struct EventLoopContext {
            core::borrowed_ptr<state::AKState> state;
            core::borrowed_ptr<PlayerEvent> event;
            core::borrowed_ptr<EvalBuffer> eval_buf;
            core::borrowed_ptr<buffer::AVBuffer> buffer;
        };

        class EventLoop final {
          public:
            explicit EventLoop();

            virtual ~EventLoop();

            void run(EventLoopContext ctx) {
                m_th = new std::thread(&EventLoop::event_thread, ctx, this);
                m_th->detach();
            }

            void set_on_thread_exit(std::function<void(void*)> on_thread_exit, void* ctx) {
                {
                    std::lock_guard<std::mutex> lock(m_on_thread_exit.mtx);
                    m_on_thread_exit.func = on_thread_exit;
                    m_on_thread_exit.ctx = ctx;
                }
                return;
            }

            void set_event_empty(bool event_empty, bool notify = false) {
                {
                    std::lock_guard<std::mutex> lock(m_state_event_empty.mtx);
                    m_state_event_empty.value = event_empty;
                }
                if (notify || !event_empty) {
                    m_state_event_empty.cv.notify_all();
                }
            }

            void wait_for_not_event_empty(void) {
                std::unique_lock<std::mutex> lock(m_state_event_empty.mtx);
                while (m_state_event_empty.value) {
                    m_state_event_empty.cv.wait(lock);
                }
            }

            void emit_inner_event(const InnerEvent& evt) { this->push(evt); }

          private:
            void free_event(InnerEvent& event);

            void push(const InnerEvent& evt) {
                {
                    std::lock_guard<std::mutex> lock(m_synced_evt.mtx);
                    m_synced_evt.queue.push_front(evt);
                }
                this->set_event_empty(false, true);
            }

            void pop(void) {
                size_t queue_size = 0;
                {
                    std::lock_guard<std::mutex> lock(m_synced_evt.mtx);
                    if (!m_synced_evt.queue.empty()) {
                        free_event(m_synced_evt.queue.back());
                        m_synced_evt.queue.pop_back();
                    }
                    queue_size = m_synced_evt.queue.size();
                }
                if (queue_size == 0) {
                    this->set_event_empty(true, true);
                }
            }

            void clear(void) {
                {
                    std::lock_guard<std::mutex> lock(m_synced_evt.mtx);
                    while (!m_synced_evt.queue.empty()) {
                        if (m_synced_evt.queue.back().ctx) {
                            free(m_synced_evt.queue.back().ctx);
                        }
                        m_synced_evt.queue.pop_back();
                    }
                }
                this->set_event_empty(true, true);
            }

            InnerEvent back(void) {
                std::lock_guard<std::mutex> lock(m_synced_evt.mtx);
                if (m_synced_evt.queue.empty()) {
                    return InnerEvent{};
                } else {
                    return m_synced_evt.queue.back();
                }
            }

            size_t size(void) {
                auto queue_size = 0;
                {
                    std::lock_guard<std::mutex> lock(m_synced_evt.mtx);
                    queue_size = m_synced_evt.queue.size();
                }
                return queue_size;
            }

          private:
            static void event_thread(EventLoopContext ctx, EventLoop* loop);

            static void pull_render_profile(EventLoopContext& ctx,
                                            core::borrowed_ptr<eval::AKEval> eval);

            static void pull_eval_buffer(const EventLoopContext& ctx,
                                         core::borrowed_ptr<eval::AKEval> eval, size_t length);

            static void seek(SeekManager& seek_mgr, const core::Rational& seek_time);

            static void hot_reload(HRManager& hr_mgr, const watch::WatchEventList& event_list);

          private:
            std::thread* m_th = nullptr;
            std::atomic<bool> m_is_alive = true;
            struct {
                std::function<void(void*)> func;
                void* ctx;
                std::mutex mtx;
            } m_on_thread_exit;

            struct {
                std::deque<InnerEvent> queue;
                std::mutex mtx;
            } m_synced_evt;

            struct {
                bool value = true;
                std::mutex mtx;
                std::condition_variable cv;
            } m_state_event_empty;
        };
    }
}
