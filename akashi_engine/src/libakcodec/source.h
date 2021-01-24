#pragma once

#include "./decode_item.h"
#include <libakcore/memory.h>
#include <libakcore/string.h>
#include <libakcore/element.h>
#include <libakcore/rational.h>

namespace akashi {
    namespace core {
        struct LayerProfile;
        struct AKAudioSpec;
        enum class VideoDecodeMethod;
    }
    namespace codec {

        struct DecodeArg;

        class LayerSource {
          public:
            explicit LayerSource() = default;
            virtual ~LayerSource() = default;

            virtual bool init(const core::LayerProfile& layer_profile,
                              const core::Rational& decode_start,
                              const core::VideoDecodeMethod& decode_method) = 0;

            virtual DecodeResult decode(const DecodeArg& decode_arg) = 0;

            virtual void finalize(void) = 0;

            virtual bool can_decode(void) const = 0;

            virtual bool done_init(void) const = 0;
        };

        class AtomSource final {
          public:
            explicit AtomSource();
            virtual ~AtomSource();

            void init(const core::AtomProfile& atom_profile, const core::Rational& decode_start,
                      const core::VideoDecodeMethod& decode_method);

            DecodeResult decode(const DecodeArg& decode_arg);

            bool can_decode(void) const { return m_can_decode; }

            bool done_init(void) const { return m_done_init; }

          private:
            bool is_layers_active(void);

          private:
            std::vector<core::owned_ptr<LayerSource>> m_layer_sources;
            core::AtomProfile m_atom_profile;
            size_t m_current_layer_idx = 0;
            size_t m_max_layer_idx = 0;
            bool m_can_decode = true;
            bool m_done_init = false;
        };

    }
}
