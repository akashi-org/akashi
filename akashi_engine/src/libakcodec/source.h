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
                              const DecodeArg& init_decode_arg) = 0;

            virtual DecodeResult decode(const DecodeArg& decode_arg) = 0;

            virtual void finalize(void) = 0;

            virtual bool can_decode(void) const = 0;

            virtual bool done_init(void) const = 0;

            virtual core::Rational dts(void) const = 0;

            virtual const core::LayerProfile& layer_profile() const = 0;

            void set_decode_halted(bool halted) { m_decode_halted = halted; }

            bool decode_halted() const { return m_decode_halted; }

          private:
            bool m_decode_halted = false;
        };

        class AtomSource final {
          public:
            explicit AtomSource();
            virtual ~AtomSource();

            void init(const core::Rational& global_duration, const core::AtomProfile& atom_profile,
                      const core::Rational& decode_start, const DecodeArg& init_decode_arg);

            DecodeResult decode(const DecodeArg& decode_arg);

            bool can_decode(void) const { return m_can_decode; }

            bool done_init(void) const { return m_done_init; }

          private:
            size_t active_layer_length(void);

            bool update_active_layers(void);

          private:
            const core::Rational BLOCK_SIZE = core::Rational(3l); // 3s

          private:
            std::vector<core::owned_ptr<LayerSource>> m_layer_sources;
            core::AtomProfile m_atom_profile;
            bool m_can_decode = true;
            bool m_done_init = false;
            DecodeArg m_init_decode_arg;
            core::Rational m_dts_src = core::Rational(0, 1);
            core::Rational m_dts_dest = core::Rational(0, 1);
            core::Rational m_global_duration = core::Rational(0, 1);

            std::vector<core::borrowed_ptr<LayerSource>> m_active_layers;
            size_t m_current_active_layer_idx = 0;
        };

    }
}
