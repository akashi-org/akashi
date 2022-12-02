#include "./event_loop.h"

#include "../event.h"
#include "../eval_buffer.h"
#include "../reload/seek_manager.h"
#include "../reload/hr_manager.h"

#include <libakcore/element.h>
#include <libakcore/memory.h>
#include <libakcore/rational.h>
#include <libakcore/logger.h>
#include <libakeval/akeval.h>
#include <libakwatch/item.h>
#include <libakstate/akstate.h>

#include <functional>
#include <deque>
#include <thread>
#include <mutex>

using namespace akashi::core;

namespace akashi {
    namespace player {

        EventLoop::EventLoop() = default;

        EventLoop::~EventLoop() = default;

        void EventLoop::close_and_wait() {
            if (m_th) {
                m_is_alive.store(false);
                this->set_event_empty(false, true);
                {
                    std::lock_guard<std::mutex> lock(m_on_thread_exit.mtx);
                    if (m_on_thread_exit.func) {
                        m_on_thread_exit.func(m_on_thread_exit.ctx);
                    }
                }
                m_th->join();
                delete m_th;
                m_th = nullptr;
            }
        }

        void EventLoop::free_event(InnerEvent& event) {
            if (!event.ctx) {
                return;
            }

            switch (event.name) {
                case InnerEventName::HOT_RELOAD: {
                    auto event_list = reinterpret_cast<watch::WatchEventList*>(event.ctx);
                    free(event_list->events);
                    free(event.ctx);
                    break;
                }
                default: {
                    free(event.ctx);
                }
            }
            event.ctx = nullptr;
        }

        void EventLoop::event_thread(EventLoopContext ctx, EventLoop* loop) {
            auto [state, event, eval_buf, buffer] = ctx;
            AKLOG_INFON("EventLoop start");

            auto eval = make_owned<eval::AKEval>(borrowed_ptr(state));

            EventLoop::pull_render_profile(ctx, borrowed_ptr(eval));

            EventLoop::pull_eval_buffer(ctx, borrowed_ptr(eval), 50);
            eval_buf->set_render_buf(eval_buf->back());
            {
                std::lock_guard<std::mutex> lock(ctx.state->m_prop_mtx);
                ctx.state->m_prop.need_first_render = true;
            }
            AKLOG_DEBUGN("Initial Render Call");
            event->emit_update();

            SeekManager seek_mgr{state, buffer, event, eval_buf, borrowed_ptr(eval)};

            HRManager hr_mgr{state, buffer, event, eval_buf, borrowed_ptr(eval)};

            while (true) {
                loop->wait_for_not_event_empty();
                if (!loop->m_is_alive.load()) {
                    break;
                }

                auto evt = loop->back();
                switch (evt.name) {
                    case InnerEventName::DUMMY: {
                        break;
                    }
                    case InnerEventName::PULL_RENDER_PROFILE: {
                        AKLOG_INFON("PULL_RENDER_PROFILE");
                        EventLoop::pull_render_profile(ctx, borrowed_ptr(eval));
                        break;
                    }
                    case InnerEventName::PULL_EVAL_BUFFER: {
                        AKLOG_INFON("PULL_EVAL_BUFFER");
                        if (ctx.eval_buf->is_hungry()) {
                            auto length = reinterpret_cast<size_t*>(evt.ctx);
                            EventLoop::pull_eval_buffer(ctx, borrowed_ptr(eval), *length);
                        }
                        break;
                    }
                    case InnerEventName::SEEK: {
                        AKLOG_INFON("SEEK");
                        auto seek_time = reinterpret_cast<Rational*>(evt.ctx);
                        EventLoop::seek(seek_mgr, *seek_time);
                        break;
                    }
                    case InnerEventName::HOT_RELOAD: {
                        AKLOG_INFON("HOT_RELOAD");
                        auto event_list = reinterpret_cast<watch::WatchEventList*>(evt.ctx);
                        EventLoop::hot_reload(hr_mgr, *event_list);
                        break;
                    }
                    case InnerEventName::INLINE_EVAL: {
                        AKLOG_INFON("INLINE_EVAL");
                        // [TODO] need to be freed somewhere else
                        auto inline_eval_ctx =
                            reinterpret_cast<InnerEventInlineEvalContext*>(evt.ctx);
                        EventLoop::inline_eval(ctx, hr_mgr, *inline_eval_ctx);
                        break;
                    }
                    default: {
                        AKLOG_ERROR("EventLoop::event_thread() invalid event name found, {}",
                                    evt.name);
                    }
                }
                loop->pop();
            }

            AKLOG_INFON("EventLoop successfully exited");
        }

        void EventLoop::pull_render_profile(EventLoopContext& ctx,
                                            core::borrowed_ptr<eval::AKEval> eval) {
            core::Path entry_path{""};
            std::string elem_name{""};
            core::Rational fps;
            {
                std::lock_guard<std::mutex> lock(ctx.state->m_prop_mtx);
                entry_path = ctx.state->m_prop.eval_state.config.entry_path;
                elem_name = ctx.state->m_prop.eval_state.config.elem_name;
                fps = ctx.state->m_prop.fps;
            }
            auto profile = eval->render_prof(entry_path.to_abspath().to_str(), elem_name);

            // [XXX] render_prof is updated in emit_set_render_prof,
            // but for the first time call of pull_eval_buffer, update render_prof here also
            {
                std::lock_guard<std::mutex> lock(ctx.state->m_prop_mtx);
                ctx.state->m_prop.render_prof = profile;
                ctx.state->m_prop.max_frame_idx =
                    ((profile.duration * fps) - Rational(1l)).to_decimal();
            }

            ctx.event->emit_set_render_prof(profile); // be careful that the decode_ready is called

            ctx.state->set_decode_layers_not_empty(core::has_layers(profile), true);
        }

        void EventLoop::pull_eval_buffer(const EventLoopContext& ctx,
                                         core::borrowed_ptr<eval::AKEval> eval, size_t length) {
            auto [state, event, eval_buf, buffer] = ctx;
            Rational fps;
            Rational duration;
            core::Path entry_path{""};
            {
                std::lock_guard<std::mutex> lock(state->m_prop_mtx);
                fps = state->m_prop.fps;
                duration = state->m_prop.render_prof.duration;
                entry_path = ctx.state->m_prop.eval_state.config.entry_path;
            }

            bool seek_completed = true;
            Rational current_time;
            {
                std::lock_guard<std::mutex> lock(state->m_prop_mtx);
                seek_completed = state->get_seek_completed();
                current_time = state->m_prop.current_time;
            }

            Rational start_time;
            if (!seek_completed) {
                start_time = current_time;
            } else {
                start_time = eval_buf->empty() ? Rational(0l) : eval_buf->front().pts;
            }

            auto is_init_pts = start_time.num() == 0 ? true : false;
            if (!is_init_pts) {
                start_time += (Rational(1, 1) / fps);
            }

            eval_buf->push(eval->eval_krons(entry_path.to_abspath().to_str(), start_time,
                                            fps.to_decimal(), duration, length));
        }

        void EventLoop::seek(SeekManager& seek_mgr, const core::Rational& seek_time) {
            seek_mgr.seek(seek_time);
        }

        void EventLoop::hot_reload(HRManager& hr_mgr, const watch::WatchEventList& event_list) {
            AKLOG_INFON("HOT Reload !!!");
            hr_mgr.reload(event_list);
        }

        void EventLoop::inline_eval(EventLoopContext& ctx, HRManager& hr_mgr,
                                    const InnerEventInlineEvalContext& inline_eval_ctx) {
            AKLOG_INFON("HOT Reload (inline eval)");
            hr_mgr.reload_inline(inline_eval_ctx);
        }

    }
}
