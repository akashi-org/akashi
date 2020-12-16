#pragma once

#include <libakcore/rational.h>

extern "C" {
#include <libavutil/rational.h>
}

struct AVFrame;

namespace akashi {
    namespace codec {

        akashi::core::Rational pts_to_rational(const int64_t pts, const AVRational& time_base);

        akashi::core::Rational rpts_to_pts(const akashi::core::Rational& rpts,
                                           const akashi::core::Rational& from,
                                           const akashi::core::Rational& start);

        struct InputSource;
        struct DecodeStream;
        class PTSSet final {
            using Rational = akashi::core::Rational;

          public:
            explicit PTSSet(const InputSource* input_src, const AVFrame* frame,
                            const int stream_index);
            virtual ~PTSSet(){};

            bool is_valid(void) const;

            bool within_range(void) const;

            const Rational& frame_pts(void) const { return m_frame_pts; };
            const Rational& frame_rpts(void) const { return m_frame_rpts; };

          private:
            int64_t calc_adjusted_pts_time(const AVFrame* frame,
                                           const DecodeStream* dec_stream) const;

          private:
            const InputSource* m_input_src = nullptr;
            int m_stream_index = 0;
            Rational m_frame_pts = Rational(-1, 1);
            Rational m_frame_rpts = Rational(-1, 1);
        };

    }
}
