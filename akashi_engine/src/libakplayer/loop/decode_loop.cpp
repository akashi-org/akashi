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
                atom_profiles = m_state->m_prop.render_prof.atom_profiles;
                decode_pts = m_state->m_prop.current_time;
                render_prof_uuid = m_state->m_prop.render_prof.uuid;
                seek_id = m_state->m_prop.seek_id;
            }
            loop_cnt = 0;
        }

        void DecodeState::seek_update(void) {
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                decode_pts = m_state->m_prop.current_time;
                seek_id = m_state->m_prop.seek_id;
            }
        }

        void DecodeState::hr_update(void) {
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                fps = m_state->m_prop.fps;
                decode_pts = m_state->m_prop.current_time;
                atom_profiles = m_state->m_prop.render_prof.atom_profiles;
                render_prof_uuid = m_state->m_prop.render_prof.uuid;
            }
            loop_cnt = 0;
        }

        void DecodeLoop::decode_thread(DecodeLoopContext ctx, DecodeLoop* loop) {
            AKLOG_INFON("Decoder thread start");

            ctx.state->wait_for_evalbuf_dequeue_ready();

            AKLOG_INFON("Decoder loop start");

            DecodeState decode_state(ctx.state);

            auto decoder =
                new codec::AKDecoder(decode_state.atom_profiles, decode_state.decode_pts);
            bool decode_finished = false;
            bool enable_loop = true;
            while (loop->m_is_alive.load() && !decode_finished) {
                ctx.state->wait_for_evalbuf_dequeue_ready();
                ctx.state->wait_for_video_decode_ready();
                ctx.state->wait_for_audio_decode_ready();
                ctx.state->wait_for_seek_completed();

                if (DecodeLoop::seek_detected(ctx.state, decode_state)) {
                    AKLOG_INFON("Decode State updated by seek");
                    decode_state.seek_update();

                    bool seek_success = true;
                    {
                        std::lock_guard<std::mutex> lock(ctx.state->m_prop_mtx);
                        seek_success = ctx.state->m_prop.seek_success;
                    }
                    if (!seek_success) {
                        delete decoder;
                        decoder = new codec::AKDecoder(decode_state.atom_profiles,
                                                       decode_state.decode_pts);
                    }
                }

                if (DecodeLoop::hr_detected(ctx.state, decode_state)) {
                    AKLOG_INFON("Decode State updated by hr");
                    decode_state.hr_update();
                    delete decoder;
                    decoder =
                        new codec::AKDecoder(decode_state.atom_profiles, decode_state.decode_pts);
                }

                auto decode_res = decoder->decode({
                    ctx.state->m_atomic_state.audio_spec.load(),
                    ctx.state->m_atomic_state.decode_method.load(),
                });

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
                        if (enable_loop) {
                            delete decoder;
                            decode_state.loop_cnt += 1;
                            decode_state.decode_pts = Rational(0, 1);
                            decoder = new codec::AKDecoder(decode_state.atom_profiles,
                                                           decode_state.decode_pts);
                        } else {
                            decode_finished = true;
                        }
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
                            decode_res.result, decode_res.layer_uuid);
                        continue;
                    }
                    case codec::DecodeResultCode::OK: {
                        switch (decode_res.buffer->prop().media_type) {
                            case buffer::AVBufferType::VIDEO: {
                                const auto comp_layer_uuid =
                                    decode_res.layer_uuid + std::to_string(decode_state.loop_cnt);

                                auto queue_size = ctx.buffer->vq->enqueue(
                                    comp_layer_uuid, std::move(decode_res.buffer));

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
                                const auto comp_layer_uuid =
                                    decode_res.layer_uuid + std::to_string(decode_state.loop_cnt);
                                ctx.buffer->aq->enqueue(comp_layer_uuid,
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
                    state->m_prop.render_prof.uuid != decode_state.render_prof_uuid;
            }
            return render_prof_updated;
        }

    }

}
