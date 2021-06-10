#include "./source.h"
#include "./decode_item.h"

#include "./backend/ffmpeg.h"

#include <libakcore/element.h>
#include <libakcore/memory.h>
#include <libakcore/logger.h>
#include <libakcore/string.h>
#include <libakbuffer/avbuffer.h>

using namespace akashi::core;

namespace akashi {

    namespace codec {

        namespace detail {

            core::Rational update_dts_avg(const core::Rational& cur_dts_avg, const size_t layer_len,
                                          const core::Rational& last_layer_dts,
                                          const core::Rational& new_layer_dts) {
                return cur_dts_avg +
                       ((new_layer_dts - last_layer_dts) / core::Rational(layer_len, 1));
            }

        }

        AtomSource::AtomSource() {}

        AtomSource::~AtomSource() {
            for (auto&& layer_source : m_layer_sources) {
                layer_source->finalize();
            }
            m_layer_sources.clear();
        }

        void AtomSource::init(const core::AtomProfile& atom_profile,
                              const core::Rational& decode_start,
                              const core::VideoDecodeMethod& decode_method,
                              const size_t video_max_queue_count) {
            m_done_init = true;
            m_atom_profile = atom_profile;
            m_max_layer_idx = atom_profile.layers.size() == 0 ? 0 : atom_profile.layers.size() - 1;

            for (size_t i = 0; i < atom_profile.layers.size(); i++) {
                m_layer_sources.push_back(make_owned<FFLayerSource>());
                m_layer_sources[i]->init(m_atom_profile.layers[i], decode_start, decode_method,
                                         video_max_queue_count);
                m_dts_avg = detail::update_dts_avg(m_dts_avg, atom_profile.layers.size(),
                                                   core::Rational(0, 1), m_layer_sources[i]->dts());
            }
        }

        DecodeResult AtomSource::decode(const DecodeArg& decode_arg) {
            DecodeResult decode_result;
            auto& cur_layer_source = m_layer_sources[m_current_layer_idx];

            if (m_current_layer_idx <= m_max_layer_idx) {
                m_current_layer_idx += 1;
                if ((cur_layer_source->dts() - m_dts_avg) > core::Rational(500, 1000)) {
                    decode_result = this->decode(decode_arg);
                } else if (cur_layer_source->can_decode()) {
                    const auto last_layer_dts = cur_layer_source->dts();
                    decode_result = cur_layer_source->decode(decode_arg);
                    m_dts_avg = detail::update_dts_avg(m_dts_avg, m_layer_sources.size(),
                                                       last_layer_dts, cur_layer_source->dts());
                } else {
                    // we need to recalculate m_dts_avg when DECODE_LAYER_ENDED
                    m_dts_avg = core::Rational(0, 1);
                    size_t active_layer_len = 0;
                    for (const auto& layer_source : m_layer_sources) {
                        if (layer_source->can_decode()) {
                            active_layer_len += 1;
                            m_dts_avg += layer_source->dts();
                        }
                    }
                    if (active_layer_len > 0) {
                        m_dts_avg = m_dts_avg / core::Rational(active_layer_len, 1);
                    }

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
