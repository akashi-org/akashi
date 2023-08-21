#include "./decode_loop.h"
#include "../event.h"

#include <libakcore/memory.h>
#include <libakcore/logger.h>
#include <libakbuffer/avbuffer.h>
#include <libakbuffer/video_queue.h>
#include <libakbuffer/audio_queue.h>

#include <libakcodec/akcodec.h>
#include <libakstate/akstate.h>

#include <thread>
#include <mutex>
#include <vector>

using namespace akashi::core;

namespace akashi {
    namespace player {

        DecodeState::DecodeState(core::borrowed_ptr<state::AKState> state) : m_state(state) {
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                fps = m_state->m_prop.fps;
                render_prof = m_state->m_prop.render_prof;
                decode_pts = m_state->m_prop.current_time;
                seek_id = m_state->m_prop.seek_id;
            }
        }

        void DecodeState::update(void) {
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                fps = m_state->m_prop.fps;
                decode_pts = m_state->m_prop.current_time;
                render_prof = m_state->m_prop.render_prof;
                seek_id = m_state->m_prop.seek_id;
            }
        }

        static bool wait_for_all_decode_ready(core::borrowed_ptr<state::AKState> state) {
            state->wait_for_kron_ready();
            state->wait_for_video_decode_ready();
            state->wait_for_audio_decode_ready();
            state->wait_for_seek_completed();
            state->wait_for_decode_layers_not_empty();
            state->wait_for_decode_loop_can_continue();

            return state->get_kron_ready() && state->get_video_decode_ready() &&
                   state->get_audio_decode_ready() && state->get_seek_completed() &&
                   state->get_decode_layers_not_empty() && state->get_decode_loop_can_continue();
        }

        void DecodeLoop::decode_thread(DecodeLoopContext ctx, DecodeLoop* loop) {
            AKLOG_INFON("Decoder thread start");

            ctx.state->wait_for_kron_ready();
            ctx.state->wait_for_decode_layers_not_empty();

            AKLOG_INFON("Decoder loop start");

            DecodeState decode_state(ctx.state);

            auto decoder = new codec::AKDecoder(decode_state.render_prof, decode_state.decode_pts);
            bool decode_finished = false;
            while (loop->m_is_alive.load() && !decode_finished) {
                if (!wait_for_all_decode_ready(ctx.state)) {
                    continue;
                }

                if (!loop->m_is_alive) {
                    break;
                }

                {
                    auto seek_detected = DecodeLoop::seek_detected(ctx.state, decode_state);
                    auto hr_detected = DecodeLoop::hr_detected(ctx.state, decode_state);
                    if (seek_detected || hr_detected) {
                        if (seek_detected) {
                            AKLOG_INFON("Decode State updated by seek");
                        }
                        if (hr_detected) {
                            AKLOG_INFON("Decode State updated by HR");
                        }
                        decode_state.update();

                        bool seek_success = true;
                        {
                            std::lock_guard<std::mutex> lock(ctx.state->m_prop_mtx);
                            seek_success = ctx.state->m_prop.seek_success;
                        }
                        if (!seek_success || hr_detected) {
                            delete decoder;
                            decoder = new codec::AKDecoder(decode_state.render_prof,
                                                           decode_state.decode_pts);
                        }
                    }
                }

                codec::DecodeArg decode_args;
                {
                    std::lock_guard<std::mutex> lock(ctx.state->m_prop_mtx);
                    decode_args.out_audio_spec = ctx.state->m_atomic_state.audio_spec.load();
                    decode_args.preferred_decode_method =
                        ctx.state->m_atomic_state.preferred_decode_method.load();
                    decode_args.video_max_queue_count = ctx.state->m_prop.video_max_queue_count;
                    decode_args.vaapi_device = ctx.state->m_video_conf.vaapi_device;
                }
                auto decode_res = decoder->decode(decode_args);

                switch (decode_res.result) {
                    case codec::DecodeResultCode::ERROR: {
                        AKLOG_ERROR("DecodeLoop::decode_thread(): decode error, code: {}",
                                    decode_res.result);
                        decode_finished = true;
                        break;
                    }
                    case codec::DecodeResultCode::DECODE_ENDED: {
                        AKLOG_INFO("DecodeLoop::decode_thread(): ended, code: {}",
                                   decode_res.result);
                        ctx.state->set_decode_loop_can_continue(false);
                        break;
                    }
                    case codec::DecodeResultCode::DECODE_LAYER_EOF:
                    case codec::DecodeResultCode::DECODE_LAYER_ENDED:
                    case codec::DecodeResultCode::DECODE_STREAM_ENDED:
                    case codec::DecodeResultCode::DECODE_ATOM_ENDED:
                    case codec::DecodeResultCode::DECODE_AGAIN:
                    case codec::DecodeResultCode::DECODE_SKIPPED: {
                        AKLOG_INFO(
                            "DecodeLoop::decode_thread(): decode skipped or layer ended, code: {}, uuid: {}",
                            decode_res.result, decode_res.layer_uuid.c_str());
                        continue;
                    }
                    case codec::DecodeResultCode::OK: {
                        switch (decode_res.buffer->prop().media_type) {
                            case buffer::AVBufferType::VIDEO: {
                                auto queue_size = ctx.buffer->vq->enqueue(
                                    decode_res.layer_uuid, std::move(decode_res.buffer));

                                bool need_first_render = false;
                                {
                                    std::lock_guard<std::mutex> lock(ctx.state->m_prop_mtx);
                                    need_first_render = ctx.state->m_prop.need_first_render;
                                }

                                if (need_first_render || queue_size == 1) {
                                    ctx.event->emit_update();
                                    AKLOG_DEBUGN("DecodeLoop::decode_atom(): Render Call");
                                }
                                break;
                            }
                            case buffer::AVBufferType::AUDIO: {
                                ctx.buffer->aq->enqueue(decode_res.layer_uuid,
                                                        std::move(decode_res.buffer));
                                break;
                            }
                            default: {
                            }
                        }

                        break;
                    }
                    default: {
                        AKLOG_ERROR(
                            "DecodeLoop::decode_thread(): invalid result code found, code: {}",
                            static_cast<int>(decode_res.result));
                        decode_finished = true;
                        break;
                    }
                }
            }

            AKLOG_INFON("Decoder loop exit");
        }

        bool DecodeLoop::seek_detected(core::borrowed_ptr<state::AKState> state,
                                       const DecodeState& decode_state) {
            bool seek_detected = false;
            {
                std::lock_guard<std::mutex> lock(state->m_prop_mtx);
                seek_detected = state->m_prop.seek_id != decode_state.seek_id;
            }
            return seek_detected;
        }

        bool DecodeLoop::hr_detected(core::borrowed_ptr<state::AKState> state,
                                     const DecodeState& decode_state) {
            bool render_prof_updated = false;
            {
                std::lock_guard<std::mutex> lock(state->m_prop_mtx);
                render_prof_updated =
                    state->m_prop.render_prof.uuid != decode_state.render_prof.uuid;
            }
            return render_prof_updated;
        }

    }

}
