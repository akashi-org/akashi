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

        using TActiveLayers = std::vector<core::borrowed_ptr<LayerSource>>;

        namespace priv {

            static core::Rational update_dts_src(const TActiveLayers& active_layers) {
                core::Rational dts_src = core::Rational(INT32_MAX, 1);
                for (const auto& layer : active_layers) {
                    if (layer->can_decode() && layer->dts() < dts_src) {
                        dts_src = layer->dts();
                    }
                }
                return dts_src;
            }

        }

        AtomSource::AtomSource() {}

        AtomSource::~AtomSource() {
            for (auto&& layer_source : m_layer_sources) {
                layer_source->finalize();
            }
            m_layer_sources.clear();
        }

        void AtomSource::init(const core::Rational& global_duration,
                              const core::AtomProfile& atom_profile,
                              const core::Rational& decode_start,
                              const core::VideoDecodeMethod& preferred_decode_method,
                              const size_t video_max_queue_count) {
            m_done_init = true;
            m_atom_profile = atom_profile;
            m_global_duration = global_duration;
            m_preferred_decode_method = preferred_decode_method;
            m_video_max_queue_count = video_max_queue_count;
            m_dts_src = decode_start;
            m_dts_dest = (std::min)(m_dts_src + this->BLOCK_SIZE, m_global_duration);

            for (size_t i = 0; i < atom_profile.av_layers.size(); i++) {
                m_layer_sources.push_back(make_owned<FFLayerSource>());
            }

            if (!this->update_active_layers()) {
                // [TODO] sane solution?
                AKLOG_ERRORN("Not found active layers");
            }
            m_current_active_layer_idx = 0;
        }

        DecodeResult AtomSource::decode(const DecodeArg& decode_arg) {
            DecodeResult decode_result;
            decode_result.atom_uuid = m_atom_profile.uuid.c_str();

            if (m_current_active_layer_idx < m_active_layers.size()) {
                auto& cur_layer_source = m_active_layers[m_current_active_layer_idx];

                if (m_dts_dest < cur_layer_source->dts()) {
                    cur_layer_source->set_decode_halted(true);
                } else {
                    cur_layer_source->set_decode_halted(false);
                }

                if (!cur_layer_source->can_decode() || cur_layer_source->decode_halted()) {
                    m_current_active_layer_idx += 1;
                    return this->decode(decode_arg);
                }

                decode_result = cur_layer_source->decode(decode_arg);

                m_current_active_layer_idx += 1;
                return decode_result;
            }

            const auto active_layer_len = this->active_layer_length();

            if (active_layer_len != 0) {
                m_dts_src = priv::update_dts_src(m_active_layers);
            }

            auto atom_eof = false;
            if (active_layer_len < 2 || (m_dts_dest - m_dts_src) < core::Rational(100, 1000)) {
                atom_eof = !this->update_active_layers();
            }

            if (atom_eof) {
                m_can_decode = false;
                decode_result.result = DecodeResultCode::DECODE_ATOM_ENDED;
                return decode_result;
            } else {
                m_current_active_layer_idx = 0;
                return this->decode(decode_arg);
            }
        }

        size_t AtomSource::active_layer_length(void) {
            size_t len = 0;
            for (const auto& layer_source : m_active_layers) {
                if (layer_source->can_decode()) {
                    len += 1;
                }
            }
            return len;
        }

        bool AtomSource::update_active_layers(void) {
            m_active_layers.clear();
            m_current_active_layer_idx = 0;

            for (size_t i = 0; i < m_atom_profile.av_layers.size(); i++) {
                const auto& layer_prof = m_atom_profile.av_layers[i];
                auto within_block = m_dts_src <= layer_prof.to && layer_prof.from <= m_dts_dest;
                if (!within_block) {
                    continue;
                }

                if (!m_layer_sources[i]->done_init()) {
                    // [TODO] is it really ok to pass m_dts_src?
                    m_layer_sources[i]->init(m_atom_profile.av_layers[i], m_dts_src,
                                             m_preferred_decode_method, m_video_max_queue_count);
                }

                if (m_layer_sources[i]->can_decode()) {
                    m_active_layers.push_back(core::borrowed_ptr(m_layer_sources[i].get()));
                }
            }

            if (m_active_layers.size() == 0) {
                if (m_dts_dest == m_global_duration) {
                    return false;
                } else {
                    m_dts_src = m_dts_dest;
                    m_dts_dest = (std::min)(m_dts_src + this->BLOCK_SIZE, m_global_duration);
                    return this->update_active_layers();
                }
            }

            m_dts_dest = (std::min)(m_dts_src + this->BLOCK_SIZE, m_global_duration);
            return true;
        }

    }

}
