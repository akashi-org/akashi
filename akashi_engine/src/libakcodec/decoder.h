#pragma once

#include <libakcore/memory.h>
#include <libakcore/rational.h>
#include <libakcore/audio.h>

#include <vector>

namespace akashi {
    namespace core {
        struct LayerProfile;
        struct AtomProfile;
        struct AKAudioSpec;
    }
    namespace buffer {
        class AVBufferData;
    }
    namespace codec {

        class AtomSource;
        class DecodeResult;
        struct DecodeArg;
        class AKDecoder final {
          public:
            explicit AKDecoder(const std::vector<core::AtomProfile>& atom_profiles,
                               const core::Rational& decode_start);
            virtual ~AKDecoder();

            DecodeResult decode(const DecodeArg& decode_arg);

          private:
            std::vector<core::AtomProfile> m_atom_profiles;
            core::Rational m_decode_start;
            std::vector<core::owned_ptr<AtomSource>> m_atom_sources;
            size_t m_current_atom_idx = 0;
            size_t m_max_atom_idx = 0;
        };

    }
}
