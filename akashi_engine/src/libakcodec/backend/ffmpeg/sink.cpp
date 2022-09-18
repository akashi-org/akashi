#include "./sink.h"

#include "./error.h"
#include "./utils.h"
#include "../../encode_item.h"

#include <libakcore/logger.h>
#include <libakstate/akstate.h>
#include <libakbuffer/hwframe.h>
#include <libakbuffer/hwframe_vaapi.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_vaapi.h>
}

#include <va/va_str.h>

using namespace akashi::core;

namespace akashi {

    namespace buffer {

        class NativeFrame {
          public:
            explicit NativeFrame(AVFrame* frame) : frame(frame){};
            virtual ~NativeFrame(){
                // if (frame) {
                //     av_frame_free(&frame);
                // }
            };
            AVFrame* frame = nullptr;
        };

        VAAPIHWFrame::VAAPIHWFrame(NativeFrame* frame, VADisplay va_display)
            : m_frame(frame), m_gfx_hwctx(nullptr) {
            m_hw_info.va_display = va_display;
            m_hw_info.va_surface_id = (uintptr_t)m_frame->frame->data[3];
        }

        VAAPIHWFrame::~VAAPIHWFrame() {
            if (m_gfx_hwctx) {
                m_gfx_hwctx.reset(nullptr);
            }
            if (m_frame) {
                delete m_frame;
            }
        }

        HWFrameInfo VAAPIHWFrame::hw_info() const { return m_hw_info; }

        void VAAPIHWFrame::set_gfx_hwctx(core::owned_ptr<GFXHWContext> gfx_hwctx) {
            if (m_gfx_hwctx) {
                AKLOG_ERRORN("Already set");
                throw std::runtime_error("Already set");
            } else {
                m_gfx_hwctx = std::move(gfx_hwctx);
            }
        }

        NativeFrame* VAAPIHWFrame::native_frame() const { return m_frame; }

        VAAPIHWFrame::NativeFrameKind VAAPIHWFrame::native_frame_kind() const {
            return NativeFrameKind::FFMPEG;
        }

    }

    namespace codec::priv {

        class FFOption final {
          public:
            explicit FFOption() = default;

            virtual ~FFOption() {
                if (m_opts) {
                    av_dict_free(&m_opts);
                    m_opts = nullptr;
                }
            }

            bool parse(const std::string& option_str) {
                if (auto err = av_dict_parse_string(&m_opts, option_str.c_str(), "=", " ", 0);
                    err < 0) {
                    AKLOG_ERROR("av_dict_parse_string() failed, ret={}", av_err2str(err));
                    return false;
                }
                return true;
            }

            void validate() {
                if (m_opts && av_dict_count(m_opts) > 0) {
                    char* format_bufstr = nullptr;
                    if (av_dict_get_string(m_opts, &format_bufstr, '=', ',') >= 0) {
                        AKLOG_WARN("Not handled format options found => {}\n", format_bufstr);
                    }
                    av_free(format_bufstr);
                }
            }

            AVDictionary** addr() {
                if (m_opts) {
                    return &m_opts;
                } else {
                    return nullptr;
                }
            }

          private:
            AVDictionary* m_opts = nullptr;
        };

    }

    namespace codec {

        FFFrameSink::FFFrameSink(core::borrowed_ptr<state::AKState> state)
            : FrameSink(state), m_state(state) {
            m_encode_method = m_state->m_encode_conf.encode_method;
        };

        FFFrameSink::~FFFrameSink() {
            // video stream
            if (m_video_stream.enc_ctx) {
                if (!m_encoder_flushed) {
                    this->flush_encoder(buffer::AVBufferType::VIDEO);
                }
                if (m_video_stream.sws_ctx) {
                    sws_freeContext(m_video_stream.sws_ctx);
                    m_video_stream.sws_ctx = nullptr;
                }
                avcodec_free_context(&m_video_stream.enc_ctx);
                m_video_stream.enc_ctx = nullptr;
                m_video_stream.enc_stream = nullptr;
            }
            // audio stream
            if (m_audio_stream.enc_ctx) {
                if (!m_encoder_flushed) {
                    this->flush_encoder(buffer::AVBufferType::AUDIO);
                }
                avcodec_free_context(&m_audio_stream.enc_ctx);
                m_audio_stream.enc_ctx = nullptr;
                m_audio_stream.enc_stream = nullptr;
            }
            m_encoder_flushed = true;
            // ofmt
            if (m_ofmt_ctx) {
                if (m_ofmt_ctx->pb) {
                    if (auto err = avio_closep(&m_ofmt_ctx->pb); err < 0) {
                        AKLOG_WARN("avio_close failed, ret={}", av_err2str(err));
                    }
                }
                avformat_close_input(&m_ofmt_ctx);
                m_ofmt_ctx = nullptr;
            }
            if (m_hw_device_ctx != nullptr) {
                av_buffer_unref(&m_hw_device_ctx);
                m_hw_device_ctx = nullptr;
            }
        }

        bool FFFrameSink::open(void) {
            // av_log_set_level(AV_LOG_VERBOSE);

            auto out_fname = m_state->m_encode_conf.out_fname;
            if (auto err = avformat_alloc_output_context2(&m_ofmt_ctx, nullptr, nullptr,
                                                          out_fname.c_str());
                err < 0) {
                AKLOG_ERROR("avformat_alloc_output_context2() failed, ret={}", av_err2str(err));
                return false;
            }

            // init hwdevice ctx if necessary
            if (m_encode_method == VideoEncodeMethod::VAAPI ||
                m_encode_method == VideoEncodeMethod::VAAPI_COPY) {
                if (auto ret = av_hwdevice_ctx_create(&m_hw_device_ctx, AV_HWDEVICE_TYPE_VAAPI,
                                                      NULL, NULL, 0);
                    ret < 0) {
                    AKLOG_ERROR("av_hwdevice_ctx_create() failed, code={}({})", AVERROR(ret),
                                av_err2str(ret));
                    return false;
                }
                {
                    auto raw_hw_device_ctx = (AVHWDeviceContext*)m_hw_device_ctx->data;
                    auto dpy =
                        static_cast<AVVAAPIDeviceContext*>(raw_hw_device_ctx->hwctx)->display;
                    AKLOG_INFO("VADisplay: {}", dpy ? "ok" : "null");
                    AKLOG_INFO("VA Vendor String: {}", vaQueryVendorString(dpy));
                }
            }

            // init streams
            if (m_state->m_encode_conf.video_codec != "" && !this->init_video_stream()) {
                return false;
            }
            if (m_state->m_encode_conf.audio_codec != "" && !this->init_audio_stream()) {
                return false;
            }

            // init io
            if (auto err = avio_open(&m_ofmt_ctx->pb, out_fname.c_str(), AVIO_FLAG_WRITE);
                err < 0) {
                AKLOG_ERROR("avio_open() failed, ret={}", av_err2str(err));
                return false;
            }

            {
                priv::FFOption format_opts;
                if (!format_opts.parse(m_state->m_encode_conf.ffmpeg_format_opts)) {
                    return false;
                }

                if (auto err = avformat_write_header(m_ofmt_ctx, format_opts.addr()); err < 0) {
                    AKLOG_ERROR("avformat_write_header() failed, ret={}", av_err2str(err));
                    return false;
                }
                format_opts.validate();
            }

            return true;
        }

        bool FFFrameSink::close(void) {
            if (m_video_stream.enc_ctx && !m_encoder_flushed) {
                this->flush_encoder(buffer::AVBufferType::VIDEO);
            }
            if (m_audio_stream.enc_ctx && !m_encoder_flushed) {
                this->flush_encoder(buffer::AVBufferType::AUDIO);
            }
            m_encoder_flushed = true;
            if (m_ofmt_ctx) {
                av_write_trailer(m_ofmt_ctx);
            }
            return true;
        }

        EncodeResultCode FFFrameSink::send(const EncodeArg& encode_arg) {
            // init avframe
            AVFrame* frame = nullptr;
            AVCodecContext* enc_ctx = nullptr;

            switch (encode_arg.type) {
                case buffer::AVBufferType::VIDEO: {
                    if (!encode_arg.hwframe) {
                        if (!this->init_video_frame(&frame, encode_arg)) {
                            av_frame_free(&frame);
                            return EncodeResultCode::ERROR;
                        }
                        if (!this->populate_video_frame(frame, encode_arg)) {
                            av_frame_free(&frame);
                            return EncodeResultCode::ERROR;
                        }
                    } else {
                        auto hwframe =
                            dynamic_cast<buffer::VAAPIHWFrame*>(encode_arg.hwframe.operator->());
                        if (!hwframe) {
                            AKLOG_ERRORN("cast failed");
                            return EncodeResultCode::ERROR;
                        }
                        frame = hwframe->native_frame()->frame;
                        frame->pts =
                            av_rescale_q(encode_arg.pts.num(), {1, (int)encode_arg.pts.den()},
                                         m_video_stream.enc_ctx->time_base);
                    }
                    enc_ctx = m_video_stream.enc_ctx;
                    break;
                }
                case buffer::AVBufferType::AUDIO: {
                    if (!this->init_audio_frame(&frame, encode_arg)) {
                        av_frame_free(&frame);
                        return EncodeResultCode::ERROR;
                    }
                    if (!this->populate_audio_frame(frame, encode_arg)) {
                        av_frame_free(&frame);
                        return EncodeResultCode::ERROR;
                    }
                    enc_ctx = m_audio_stream.enc_ctx;
                    break;
                }
                default: {
                    AKLOG_ERROR("Invalid type found, {}", encode_arg.type);
                    av_frame_free(&frame);
                    return EncodeResultCode::ERROR;
                }
            }

            AVFrame* proxy_frame = frame;

            if (m_encode_method == VideoEncodeMethod::VAAPI_COPY && m_video_stream.enc_ctx &&
                encode_arg.type == buffer::AVBufferType::VIDEO) {
                // create hwframe
                auto hwframe = av_frame_alloc();
                if (!hwframe) {
                    AKLOG_ERRORN("av_frame_alloc() failed");
                    return EncodeResultCode::ERROR;
                }

                hwframe->width = m_video_stream.enc_ctx->width;
                hwframe->height = m_video_stream.enc_ctx->height;
                // hwframe->format = m_video_stream.enc_ctx->pix_fmt;
                hwframe->pts = frame->pts;

                if (auto err =
                        av_hwframe_get_buffer(m_video_stream.enc_ctx->hw_frames_ctx, hwframe, 0);
                    err < 0) {
                    AKLOG_ERROR("av_hwframe_get_buffer() failed, ret={}", av_err2str(err));
                    return EncodeResultCode::ERROR;
                }
                if (!hwframe->hw_frames_ctx) {
                    AKLOG_ERRORN("hw_frames_ctx for AVFrame is null");
                    return EncodeResultCode::ERROR;
                }

                if (auto err = av_hwframe_transfer_data(hwframe, frame, 0); err < 0) {
                    AKLOG_ERROR("av_hwframe_transfer_data() failed, ret={}", av_err2str(err));
                    return EncodeResultCode::ERROR;
                }

                if (auto err = av_frame_copy_props(hwframe, frame); err < 0) {
                    AKLOG_ERROR("av_frame_copy_props() failed, ret={}", av_err2str(err));
                    return EncodeResultCode::ERROR;
                }

                proxy_frame = hwframe;

                if (frame) {
                    av_frame_free(&frame);
                }
            }

            // send_frame
            if (auto err = avcodec_send_frame(enc_ctx, proxy_frame); err < 0) {
                av_frame_free(&frame);
                if (err == AVERROR(EAGAIN)) {
                    return EncodeResultCode::SEND_EAGAIN;
                } else {
                    AKLOG_ERROR("avcodec_send_frame() failed, ret={}", av_err2str(err));
                    return EncodeResultCode::ERROR;
                }
            }

            if (proxy_frame) {
                av_frame_free(&proxy_frame);
            }

            return EncodeResultCode::OK;
        };

        EncodeWriteResult FFFrameSink::write(const EncodeWriteArg& write_arg) {
            AVPacket* pkt = av_packet_alloc();
            EncodeResultCode result = EncodeResultCode::NONE;
            AVCodecContext* enc_ctx = nullptr;
            AVStream* enc_stream = nullptr;

            switch (write_arg.type) {
                case buffer::AVBufferType::VIDEO: {
                    enc_ctx = m_video_stream.enc_ctx;
                    enc_stream = m_video_stream.enc_stream;
                    break;
                }
                case buffer::AVBufferType::AUDIO: {
                    enc_ctx = m_audio_stream.enc_ctx;
                    enc_stream = m_audio_stream.enc_stream;
                    break;
                }
                default: {
                    AKLOG_ERROR("Invalid type found, {}", write_arg.type);
                    result = EncodeResultCode::ERROR;
                    goto exit;
                }
            }

            // recv pkt and write it to file
            {
                auto err = avcodec_receive_packet(enc_ctx, pkt);
                if (err == AVERROR(EAGAIN)) {
                    result = EncodeResultCode::RECV_EAGAIN;
                    goto exit;
                } else if (err == AVERROR_EOF) {
                    result = EncodeResultCode::RECV_EOF;
                    goto exit;
                } else if (err < 0) {
                    AKLOG_ERROR("avcodec_receive_packet() failed, ret={}", av_err2str(err));
                    result = EncodeResultCode::ERROR;
                    goto exit;
                }

                pkt->stream_index = enc_stream->index;
                av_packet_rescale_ts(pkt, enc_ctx->time_base, enc_stream->time_base);

                auto pkt_pts = Rational(pkt->pts) * to_rational(enc_stream->time_base);

                if (auto err = av_interleaved_write_frame(m_ofmt_ctx, pkt); err < 0) {
                    AKLOG_ERROR("av_interleaved_write_frame() failed, ret={}", av_err2str(err));
                    // break or return?
                }

                AKLOG_WARN("Frame (type: {}, pts: {}) written", write_arg.type,
                           pkt_pts.to_decimal());

                // [TODO] really necessary?
                // see av_interleaved_write_frame()'s documentation
                // av_packet_unref(pkt);
            }
            result = EncodeResultCode::OK;

        exit:
            if (pkt) {
                av_packet_free(&pkt);
            }
            return {.result = result};
        }

        size_t FFFrameSink::nb_samples_per_frame(void) {
            if (m_audio_stream.enc_ctx) {
                return m_audio_stream.enc_ctx->frame_size;
            }
            // in case where AV_CODEC_CAP_VARIABLE_FRAME_SIZE is set
            else {
                if (m_video_stream.enc_ctx) {
                    Rational fps;
                    int sample_rate;
                    {
                        std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                        fps = m_state->m_prop.fps;
                        sample_rate = m_state->m_atomic_state.encode_audio_spec.load().sample_rate;
                    }
                    AKLOG_WARNN(
                        "Could not find the appropriate nb_samples per frame. Using calculated number for it");
                    return static_cast<size_t>(
                        ((Rational(1l) / fps) * Rational(sample_rate, 1)).to_decimal());
                } else {
                    AKLOG_WARNN(
                        "Could not find the appropriate nb_samples per frame. Using arbitrary number for it");
                    return 256;
                }
            }
        }

        core::AKAudioSampleFormat
        FFFrameSink::validate_audio_format(const core::AKAudioSampleFormat& sample_format) {
            // find codec
            auto codec = avcodec_find_encoder_by_name(m_state->m_encode_conf.audio_codec.c_str());
            if (!codec) {
                AKLOG_ERROR("avcodec_find_encoder_by_name() failed, codec_name: {}",
                            m_state->m_encode_conf.audio_codec.c_str());
                return core::AKAudioSampleFormat::NONE;
            }

            const enum AVSampleFormat* p = codec->sample_fmts;
            auto ff_sample_fmt = to_ff_sample_format(sample_format);
            auto ff_sample_fmt_alt = av_get_alt_sample_fmt(
                ff_sample_fmt, av_sample_fmt_is_planar(ff_sample_fmt) ? 0 : 1);
            bool success = false;
            bool alt_success = false;
            while (*p != AV_SAMPLE_FMT_NONE) {
                if (*p == ff_sample_fmt) {
                    success = true;
                    break;
                }
                if (*p == ff_sample_fmt_alt) {
                    alt_success = true;
                }
                p++;
            }
            if (!success) {
                if (alt_success) {
                    AKLOG_INFO("Using alt sample format `{}`",
                               av_get_sample_fmt_name(ff_sample_fmt_alt));
                    return from_ff_sample_format(ff_sample_fmt_alt);
                }
                AKLOG_ERROR("Not supported sample format `{}` found",
                            av_get_sample_fmt_name(ff_sample_fmt));
                return core::AKAudioSampleFormat::NONE;
            } else {
                return from_ff_sample_format(ff_sample_fmt);
            }
        }

        std::unique_ptr<buffer::HWFrame> FFFrameSink::create_hwframe(void) {
            if (!m_video_stream.enc_ctx) {
                AKLOG_ERRORN("AVCodecContext for video streams is null");
                return nullptr;
            }
            if (!m_hw_device_ctx) {
                AKLOG_ERRORN("m_hw_device_ctx is null");
                return nullptr;
            }

            auto frame = av_frame_alloc();
            if (!frame) {
                AKLOG_ERRORN("av_frame_alloc() failed");
                return nullptr;
            }

            // [TODO] format, colorspace?
            frame->width = m_video_stream.enc_ctx->width;
            frame->height = m_video_stream.enc_ctx->height;
            frame->format = AV_PIX_FMT_NV12;
            // frame->colorspace = AVColorSpace::AVCOL_SPC_BT709;
            // frame->color_range = AVColorRange::AVCOL_RANGE_MPEG;

            // frame->format = AV_PIX_FMT_RGBA;
            // frame->colorspace = AVColorSpace::AVCOL_SPC_RGB;
            // frame->color_range = AVColorRange::AVCOL_RANGE_JPEG;

            if (auto err = av_hwframe_get_buffer(m_video_stream.enc_ctx->hw_frames_ctx, frame, 0);
                err < 0) {
                AKLOG_ERROR("av_hwframe_get_buffer() failed, ret={}", av_err2str(err));
                return nullptr;
            }
            if (!frame->hw_frames_ctx) {
                AKLOG_ERRORN("hw_frames_ctx for AVFrame is null");
                return nullptr;
            }

            auto raw_hw_device_ctx = (AVHWDeviceContext*)m_hw_device_ctx->data;
            auto va_display = static_cast<AVVAAPIDeviceContext*>(raw_hw_device_ctx->hwctx)->display;

            return std::make_unique<buffer::VAAPIHWFrame>(new buffer::NativeFrame{frame},
                                                          va_display);
        }

        bool FFFrameSink::init_video_stream() {
            // find codec
            auto codec = avcodec_find_encoder_by_name(m_state->m_encode_conf.video_codec.c_str());
            if (!codec) {
                AKLOG_ERROR("avcodec_find_encoder_by_name() failed, codec_name: {}",
                            m_state->m_encode_conf.video_codec.c_str());
                return false;
            }

            // alloc codec ctx
            m_video_stream.enc_ctx = avcodec_alloc_context3(codec);
            if (!m_video_stream.enc_ctx) {
                AKLOG_ERRORN("avcodec_alloc_context3() failed");
                return false;
            }
            auto enc_ctx = m_video_stream.enc_ctx;

            // codec ctx settings
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                enc_ctx->width = m_state->m_prop.video_width;
                enc_ctx->height = m_state->m_prop.video_height;
                enc_ctx->time_base = to_av_rational(Rational(1l) / m_state->m_prop.fps);
                enc_ctx->framerate = to_av_rational(m_state->m_prop.fps);
                enc_ctx->colorspace = AVColorSpace::AVCOL_SPC_BT709;
                // enc_ctx->color_primaries = AVColorPrimaries::AVCOL_PRI_BT709;
                // enc_ctx->color_trc = AVColorTransferCharacteristic::AVCOL_TRC_BT709;
                enc_ctx->color_range = AVColorRange::AVCOL_RANGE_MPEG;

                if (m_encode_method == VideoEncodeMethod::VAAPI ||
                    m_encode_method == VideoEncodeMethod::VAAPI_COPY) {
                    enc_ctx->pix_fmt = AV_PIX_FMT_VAAPI;
                } else {
                    // [TODO] add a field for AKState
                    enc_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
                }

                // [XXX] settings for interlacing?
            }

            if (m_ofmt_ctx->flags & AVFMT_GLOBALHEADER) {
                enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
            }

            if (m_hw_device_ctx) {
                AVBufferRef* hw_frames_ref = nullptr;
                AVHWFramesContext* hw_frames_ctx = nullptr;

                hw_frames_ref = av_hwframe_ctx_alloc(m_hw_device_ctx);
                if (!hw_frames_ref) {
                    AKLOG_ERRORN("Failed to alloc AVHWFramesContext");
                    return false;
                }

                hw_frames_ctx = (AVHWFramesContext*)(hw_frames_ref->data);
                hw_frames_ctx->format = AV_PIX_FMT_VAAPI;
                hw_frames_ctx->sw_format = AV_PIX_FMT_NV12;
                hw_frames_ctx->width = enc_ctx->width;
                hw_frames_ctx->height = enc_ctx->height;

                // [XXX] use an arbitrary number
                hw_frames_ctx->initial_pool_size = 20;
                hw_frames_ctx->pool = nullptr;

                if (auto err = av_hwframe_ctx_init(hw_frames_ref); err < 0) {
                    AKLOG_ERROR("av_hwframe_ctx_init() failed, ret={}", av_err2str(err));
                    av_buffer_unref(&hw_frames_ref);
                    return false;
                }

                enc_ctx->hw_frames_ctx = av_buffer_ref(hw_frames_ref);
                if (!enc_ctx->hw_frames_ctx) {
                    AKLOG_ERRORN("AVHWFramesContext reference create failed");
                    av_buffer_unref(&hw_frames_ref);
                    return false;
                }

                // auto raw_hw_device_ctx = (AVHWDeviceContext*)m_hw_device_ctx->data;
                // VADisplay dpy =
                // static_cast<AVVAAPIDeviceContext*>(raw_hw_device_ctx->hwctx)->display; auto
                // num_profiles = vaMaxNumProfiles(dpy); auto profiles =
                // static_cast<VAProfile*>(calloc(num_profiles, sizeof(VAProfile))); if (auto status
                // = vaQueryConfigProfiles(dpy, profiles, &num_profiles);
                //     status != VA_STATUS_SUCCESS) {
                //     AKLOG_ERROR( "Failed to query va profiles: %s\n", vaErrorStr(status));
                // }
                // for (int i = 0; i < num_profiles; i++) {
                //     AKLOG_ERROR( "%s\n", vaProfileStr(profiles[i]));
                // }

                av_buffer_unref(&hw_frames_ref);
            }

            {
                priv::FFOption codec_opts;
                if (!codec_opts.parse(m_state->m_encode_conf.video_ffmpeg_codec_opts)) {
                    return false;
                }

                // open encoder
                if (auto err = avcodec_open2(enc_ctx, codec, codec_opts.addr()); err < 0) {
                    AKLOG_ERROR("avcodec_open2() failed, ret={}", av_err2str(err));
                    return false;
                }
                codec_opts.validate();
            }

            // init stream
            m_video_stream.enc_stream = avformat_new_stream(m_ofmt_ctx, codec);
            if (!m_video_stream.enc_stream) {
                AKLOG_ERRORN("avformat_new_stream() failed");
                return false;
            }

            if (auto err =
                    avcodec_parameters_from_context(m_video_stream.enc_stream->codecpar, enc_ctx);
                err < 0) {
                AKLOG_ERROR("avcodec_parameters_from_context() failed, ret={}", av_err2str(err));
                return false;
            }

            if (m_encode_method != VideoEncodeMethod::VAAPI) {
                auto dst_pixfmt = m_encode_method == VideoEncodeMethod::VAAPI_COPY
                                      ? AV_PIX_FMT_NV12
                                      : enc_ctx->pix_fmt;

                // init sws ctx
                // clang-format off
                m_video_stream.sws_ctx = sws_getCachedContext(nullptr,
                    // src
                    enc_ctx->width, enc_ctx->height, AV_PIX_FMT_RGB24,
                    // dst
                    enc_ctx->width, enc_ctx->height, dst_pixfmt,
                    // flags
                    SWS_BICUBIC | SWS_FULL_CHR_H_INP | SWS_FULL_CHR_H_INT | SWS_ACCURATE_RND, // SWS_LANCZOS | SWS_FULL_CHR_H_INT | SWS_ACCURATE_RND,
                    // options
                    nullptr, nullptr, nullptr
                );
                // clang-format on
                if (!m_video_stream.sws_ctx) {
                    AKLOG_ERRORN("sws_getCashedContext() failed");
                    return false;
                }

                // NB: This operation is really important for handling colorspace issues properly
                if (auto err = sws_setColorspaceDetails(
                        m_video_stream.sws_ctx, sws_getCoefficients(SWS_CS_DEFAULT), 1,
                        sws_getCoefficients(SWS_CS_ITU709), 0, 0, (1 << 16), (1 << 16));
                    err < 0) {
                    AKLOG_ERRORN("sws_setColorspaceDetails() failed");
                }
            }

            return true;
        }

        bool FFFrameSink::init_audio_stream() {
            // find codec
            auto codec = avcodec_find_encoder_by_name(m_state->m_encode_conf.audio_codec.c_str());
            if (!codec) {
                AKLOG_ERROR("avcodec_find_encoder_by_name() failed, codec_name: {}",
                            m_state->m_encode_conf.audio_codec.c_str());
                return false;
            }

            // alloc codec ctx
            m_audio_stream.enc_ctx = avcodec_alloc_context3(codec);
            if (!m_audio_stream.enc_ctx) {
                AKLOG_ERRORN("avcodec_alloc_context3() failed");
                return false;
            }
            auto enc_ctx = m_audio_stream.enc_ctx;

            // codec ctx settings
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);

                enc_ctx->sample_rate = m_state->m_atomic_state.encode_audio_spec.load().sample_rate;
                enc_ctx->channel_layout = to_ff_channel_layout(
                    m_state->m_atomic_state.encode_audio_spec.load().channel_layout);
                enc_ctx->channels = av_get_channel_layout_nb_channels(enc_ctx->channel_layout);
                enc_ctx->sample_fmt =
                    to_ff_sample_format(m_state->m_atomic_state.encode_audio_spec.load().format);
                enc_ctx->time_base = {1, enc_ctx->sample_rate};
                // [XXX] settings for other params(bit_rate, ...)
            }

            if (m_ofmt_ctx->flags & AVFMT_GLOBALHEADER) {
                enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
            }

            {
                priv::FFOption codec_opts;
                if (!codec_opts.parse(m_state->m_encode_conf.audio_ffmpeg_codec_opts)) {
                    return false;
                }

                // open encoder
                if (auto err = avcodec_open2(enc_ctx, codec, codec_opts.addr()); err < 0) {
                    AKLOG_ERROR("avcodec_open2() failed, ret={}", av_err2str(err));
                    return false;
                }
                codec_opts.validate();
            }

            // init stream
            m_audio_stream.enc_stream = avformat_new_stream(m_ofmt_ctx, codec);
            if (!m_audio_stream.enc_stream) {
                AKLOG_ERRORN("avformat_new_stream() failed");
                return false;
            }

            if (auto err =
                    avcodec_parameters_from_context(m_audio_stream.enc_stream->codecpar, enc_ctx);
                err < 0) {
                AKLOG_ERROR("avcodec_parameters_from_context() failed, ret={}", av_err2str(err));
                return false;
            }

            return true;
        }

        bool FFFrameSink::init_video_frame(AVFrame** frame, const EncodeArg& encode_arg) {
            // alloc frame
            *frame = av_frame_alloc();
            if (!(*frame)) {
                AKLOG_ERRORN("av_frame_alloc() failed");
                return false;
            }

            // frame settings
            (*frame)->width = m_video_stream.enc_ctx->width;
            (*frame)->height = m_video_stream.enc_ctx->height;

            if (m_encode_method == VideoEncodeMethod::VAAPI_COPY) {
                (*frame)->format = AV_PIX_FMT_NV12;
            } else {
                (*frame)->format = m_video_stream.enc_ctx->pix_fmt;
            }

            // [TODO] settings for interlacing?
            // [TODO] is this really necessary?
            (*frame)->pts = av_rescale_q(encode_arg.pts.num(), {1, (int)encode_arg.pts.den()},
                                         m_video_stream.enc_ctx->time_base);

            (*frame)->colorspace = m_video_stream.enc_ctx->colorspace;
            // (*frame)->colorspace = AVColorSpace::AVCOL_SPC_RGB;
            // (*frame)->color_range = AVColorRange::AVCOL_RANGE_JPEG;

            // alloc video buffer
            if (auto err = av_frame_get_buffer(*frame, 0); err < 0) {
                AKLOG_ERROR("av_frame_get_buffer() failed, ret={}", av_err2str(err));
                return false;
            }

            return true;
        }

        bool FFFrameSink::init_audio_frame(AVFrame** frame, const EncodeArg& encode_arg) {
            // alloc frame
            *frame = av_frame_alloc();
            if (!(*frame)) {
                AKLOG_ERRORN("av_frame_alloc() failed");
                return false;
            }

            // frame settings
            (*frame)->nb_samples = encode_arg.nb_samples;
            (*frame)->format = m_audio_stream.enc_ctx->sample_fmt;
            (*frame)->channel_layout = m_audio_stream.enc_ctx->channel_layout;
            // [TODO] is this really necessary?
            (*frame)->pts = av_rescale_q(encode_arg.pts.num(), {1, (int)encode_arg.pts.den()},
                                         m_audio_stream.enc_ctx->time_base);

            // alloc audio buffer
            if (auto err = av_frame_get_buffer(*frame, 0); err < 0) {
                AKLOG_ERROR("av_frame_get_buffer() failed, ret={}", av_err2str(err));
                return false;
            }

            return true;
        }

        bool FFFrameSink::populate_video_frame(AVFrame* frame, const EncodeArg& encode_arg) {
            uint8_t* src_slice[4] = {encode_arg.buffer.get(), 0, 0, 0};
            int src_linesize[4] = {
                av_image_get_linesize(AV_PIX_FMT_RGB24, m_video_stream.enc_ctx->width, 0), 0, 0, 0};
            // clang-format off
            sws_scale(m_video_stream.sws_ctx, 
              // src
              src_slice, src_linesize, 0, m_video_stream.enc_ctx->height, 
              // dst
              frame->data, frame->linesize 
            );
            // clang-format on
            return true;
        }

        bool FFFrameSink::populate_audio_frame(AVFrame* frame, const EncodeArg& encode_arg) {
            if (auto err =
                    avcodec_fill_audio_frame(frame, frame->channels, (AVSampleFormat)frame->format,
                                             encode_arg.buffer.get(), encode_arg.buf_size, 0);
                err < 0) {
                AKLOG_ERROR("avcodec_fill_audio_frame() failed, ret={}", av_err2str(err));
                return false;
            }
            return true;
        }

        void FFFrameSink::flush_encoder(const buffer::AVBufferType& type) {
            AVCodecContext* enc_ctx;
            switch (type) {
                case buffer::AVBufferType::VIDEO: {
                    enc_ctx = m_video_stream.enc_ctx;
                    break;
                }
                case buffer::AVBufferType::AUDIO: {
                    enc_ctx = m_audio_stream.enc_ctx;
                    break;
                }
                default: {
                    AKLOG_ERROR("Invalid type found, {}", type);
                    return;
                }
            }

            if (!enc_ctx) {
                return;
            }

            avcodec_send_frame(enc_ctx, nullptr);

            while (true) {
                auto write_result = this->write({type});
                switch (write_result.result) {
                    case codec::EncodeResultCode::ERROR: {
                        AKLOG_ERRORN("Encode error while flushing");
                        return;
                    }
                    case codec::EncodeResultCode::RECV_EOF: {
                        AKLOG_INFON("Successfully flushed the encoder");
                        return;
                    }
                    default: {
                        break;
                    }
                }
            }
        }

    }
}
