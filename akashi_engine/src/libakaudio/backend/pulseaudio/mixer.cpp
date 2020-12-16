#include "./mixer.h"

#include <libakcore/logger.h>

#include <pulse/pulseaudio.h>

using namespace akashi::core;

namespace akashi {
    namespace audio {

        void mix_layer(uint8_t* buffer, const size_t bytes_to_fill, uint8_t* audio_data,
                       const core::LayerProfile&) {
            for (size_t i = 0; i < bytes_to_fill; i += 4) {
                float old_v = *(float*)(&buffer[i]);
                // float new_v = *(float*)(&audio_data[i]) * layer.gain;
                float new_v = *(float*)(&audio_data[i]) * 1;
                float mix_gain = 1.0;
                float res = old_v + (mix_gain * new_v);

                for (size_t j = 0; j < sizeof(float); j++) {
                    buffer[i + j] = ((uint8_t*)&res)[j];
                }
            }
        }

        void adjust_volume(uint8_t* buffer, const size_t buf_size, const double volume) {
            for (size_t i = 0; i < buf_size; i += 4) {
                float old_v = *(float*)(&buffer[i]);
                float res = volume * old_v;

                for (size_t j = 0; j < sizeof(float); j++) {
                    buffer[i + j] = ((uint8_t*)&res)[j];
                }
            }
        }

    }
}
