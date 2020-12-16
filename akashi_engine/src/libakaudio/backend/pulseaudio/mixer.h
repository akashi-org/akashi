#pragma once

#include <cstdint>
#include <cstddef>

namespace akashi {
    namespace core {
        class LayerProfile;
    }
    namespace audio {

        void mix_layer(uint8_t* buffer, const size_t bytes_to_fill, uint8_t* audio_data,
                       const core::LayerProfile& layer);

        void adjust_volume(uint8_t* buffer, const size_t buf_size, const double volume);

    }
}
