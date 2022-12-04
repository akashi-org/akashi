#include "./decoder.h"

#include <libakcore/logger.h>
#include <libakcore/element.h>
#include <libakcore/rational.h>
#include <libakcore/memory.h>
#include <libakcore/path.h>
#include <libakstate/akstate.h>
#include <libakcodec/akcodec.h>
#include <libakbuffer/avbuffer.h>
#include <libakbuffer/video_queue.h>
#include <libakbuffer/audio_queue.h>
#include <libakbuffer/audio_buffer.h>

using namespace akashi::core;

namespace akashi {
    namespace encoder {

        DecodeResult exec_decode(DecodeParams& decode_params) {
            auto [state, decoder, buffer, abuffer] = decode_params;
            codec::DecodeArg decode_args;
            {
                std::lock_guard<std::mutex> lock(state->m_prop_mtx);
                decode_args.out_audio_spec = state->m_atomic_state.audio_spec.load();
                decode_args.preferred_decode_method =
                    state->m_atomic_state.preferred_decode_method.load();
                // [TODO] this value should be changed
                decode_args.video_max_queue_count = state->m_prop.video_max_queue_count;
                decode_args.vaapi_device = state->m_video_conf.vaapi_device;
            }

            while (true) {
                if (!state->get_video_decode_ready() || !abuffer->write_ready()) {
                    break;
                }
                auto decode_res = decoder->decode(decode_args);

                switch (decode_res.result) {
                    case codec::DecodeResultCode::ERROR: {
                        AKLOG_ERROR("decode error, code: {}", decode_res.result);
                        return DecodeResult::ERR;
                    }
                    case codec::DecodeResultCode::DECODE_ENDED: {
                        AKLOG_INFO("DecodeLoop::decode_thread(): ended, code: {}",
                                   decode_res.result);
                        return DecodeResult::ENDED;
                    }
                    case codec::DecodeResultCode::DECODE_LAYER_EOF:
                    case codec::DecodeResultCode::DECODE_LAYER_ENDED:
                    case codec::DecodeResultCode::DECODE_STREAM_ENDED:
                    case codec::DecodeResultCode::DECODE_ATOM_ENDED:
                    case codec::DecodeResultCode::DECODE_AGAIN:
                    case codec::DecodeResultCode::DECODE_SKIPPED: {
                        AKLOG_INFO("decode skipped or layer ended, code: {}, uuid: {}",
                                   decode_res.result, decode_res.layer_uuid.c_str());
                        break;
                    }
                    case codec::DecodeResultCode::OK: {
                        switch (decode_res.buffer->prop().media_type) {
                            case buffer::AVBufferType::VIDEO: {
                                if (state->m_encode_conf.video_codec != "") {
                                    const auto comp_layer_uuid = decode_res.layer_uuid;
                                    buffer->vq->enqueue(comp_layer_uuid,
                                                        std::move(decode_res.buffer));
                                }
                                break;
                            }
                            case buffer::AVBufferType::AUDIO: {
                                if (state->m_encode_conf.audio_codec != "") {
                                    const auto comp_layer_uuid = decode_res.layer_uuid;
                                    auto pts = decode_res.buffer->prop().pts;
                                    abuffer->enqueue(std::move(decode_res.buffer));
                                    AKLOG_INFO("AudioBuffer Enqueued, pts: {}, id: {}",
                                               pts.to_decimal(), comp_layer_uuid);
                                }
                                break;
                            }
                            default: {
                            }
                        }

                        break;
                    }
                    default: {
                        AKLOG_ERROR("invalid result code found, code: {}",
                                    static_cast<int>(decode_res.result));
                        return DecodeResult::ERR;
                    }
                }
            }
            return DecodeResult::OK;
        }

    }
}
