#include "./decoder.h"
#include "./decode_item.h"
#include "./source.h"

#include "./backend/ffmpeg.h"

#include <libakbuffer/avbuffer.h>
#include <libakcore/memory.h>
#include <libakcore/logger.h>

using namespace akashi::core;

namespace akashi {
    namespace codec {

        AKDecoder::AKDecoder(const std::vector<core::AtomProfile>& atom_profiles,
                             const core::Rational& decode_start) {
            m_atom_profiles = atom_profiles;
            m_decode_start = decode_start;

            // find the appropriate current_atom_idx for m_decode_start
            for (const auto& atom_profile : m_atom_profiles) {
                if (to_rational(atom_profile.from) <= m_decode_start &&
                    m_decode_start <= to_rational(atom_profile.to)) {
                    break;
                }
                m_current_atom_idx += 1;
            }

            m_max_atom_idx = atom_profiles.size() == 0 ? 0 : atom_profiles.size() - 1;

            for (size_t i = 0; i < atom_profiles.size(); i++) {
                m_atom_sources.push_back(make_owned<AtomSource>());
            }
        }

        AKDecoder::~AKDecoder() { m_atom_sources.clear(); }

        DecodeResult AKDecoder::decode(const DecodeArg& decode_arg) {
            auto& cur_atom_source = m_atom_sources[m_current_atom_idx];
            if (!cur_atom_source->done_init()) {
                cur_atom_source->init(m_atom_profiles[m_current_atom_idx], m_decode_start,
                                      decode_arg.decode_method, decode_arg.video_max_queue_count);
            }

            if (cur_atom_source->can_decode()) {
                return cur_atom_source->decode(decode_arg);
            }

            if (m_current_atom_idx < m_max_atom_idx) {
                m_current_atom_idx += 1;
                return this->decode(decode_arg);
            } else {
                DecodeResult decode_result;
                decode_result.result = DecodeResultCode::DECODE_ENDED;
                return decode_result;
            }
        }

    }
}
