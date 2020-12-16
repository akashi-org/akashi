#include "./source.h"
#include "./decode_item.h"

#include "./backend/ffmpeg.h"

#include <libakcore/element.h>
#include <libakcore/memory.h>
#include <libakcore/string.h>
#include <libakbuffer/avbuffer.h>

using namespace akashi::core;

namespace akashi {

    namespace codec {

        AtomSource::AtomSource() {}

        AtomSource::~AtomSource() {
            for (auto&& layer_source : m_layer_sources) {
                layer_source->finalize();
            }
            m_layer_sources.clear();
        }

        void AtomSource::init(const core::AtomProfile& atom_profile,
                              const core::Rational& decode_start) {
            m_done_init = true;
            m_atom_profile = atom_profile;
            m_max_layer_idx = atom_profile.layers.size() == 0 ? 0 : atom_profile.layers.size() - 1;

            for (size_t i = 0; i < atom_profile.layers.size(); i++) {
                m_layer_sources.push_back(make_owned<FFLayerSource>());
                m_layer_sources[i]->init(m_atom_profile.layers[i], decode_start);
            }
        }

        DecodeResult AtomSource::decode(const DecodeArg& decode_arg) {
            DecodeResult decode_result;
            auto& cur_layer_source = m_layer_sources[m_current_layer_idx];

            if (m_current_layer_idx <= m_max_layer_idx) {
                m_current_layer_idx += 1;
                if (cur_layer_source->can_decode()) {
                    decode_result = cur_layer_source->decode(decode_arg);
                } else {
                    decode_result = this->decode(decode_arg);
                }
            } else {
                if (this->is_layers_active()) {
                    m_current_layer_idx = 0;
                    decode_result = this->decode(decode_arg);
                } else {
                    m_can_decode = false;
                    decode_result.result = DecodeResultCode::DECODE_ATOM_ENDED;
                }
            }

            decode_result.atom_uuid = m_atom_profile.uuid.c_str();
            return decode_result;
        }

        bool AtomSource::is_layers_active(void) {
            for (const auto& layer_source : m_layer_sources) {
                if (layer_source->can_decode()) {
                    return true;
                }
            }
            return false;
        }

    }

}
