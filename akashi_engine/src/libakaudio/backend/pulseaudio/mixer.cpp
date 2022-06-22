#include "./mixer.h"

#include <libakcore/logger.h>
#include <libakcore/element.h>

#include <pulse/pulseaudio.h>

using namespace akashi::core;

namespace akashi {
    namespace audio {

        static float parse_float(const uint8_t* buf) {
            float value = 0.0f;
            memcpy(&value, buf, sizeof(float));
            return value;
        }

        void mix_layer(uint8_t* buffer, const size_t bytes_to_fill, uint8_t* audio_data,
                       const core::LayerProfile& layer) {
            for (size_t i = 0; i < bytes_to_fill; i += 4) {
                float old_v = parse_float(&buffer[i]);
                float new_v = parse_float(&audio_data[i]) * layer.gain;
                float mix_gain = 1.0;
                float res = old_v + (mix_gain * new_v);

                for (size_t j = 0; j < sizeof(float); j++) {
                    buffer[i + j] = ((uint8_t*)&res)[j];
                }
            }
        }

        double adjust_volume(uint8_t* buffer, const size_t buf_size, const double volume) {
            double rms_values = 0.0;
            for (size_t i = 0; i < buf_size; i += 4) {
                float old_v = parse_float(&buffer[i]);
                rms_values += old_v * old_v;
                float res = volume * old_v;

                for (size_t j = 0; j < sizeof(float); j++) {
                    buffer[i + j] = ((uint8_t*)&res)[j];
                }
            }

            double rms = std::sqrt(rms_values / (buf_size / 4.0));
            return rms;
        }

    }
}
