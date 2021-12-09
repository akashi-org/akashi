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

        namespace detail {

            static core::Rational calc_dts_avg(const TActiveLayers& active_layers) {
                core::Rational avg = core::Rational(0, 1);
                for (const auto& layer : active_layers) {
                    avg += layer->dts();
                }
                return avg / core::Rational(active_layers.size(), 1);
            }

            static core::Rational update_dts_avg(const core::Rational& cur_dts_avg,
                                                 const size_t layer_len,
                                                 const core::Rational& last_layer_dts,
                                                 const core::Rational& new_layer_dts) {
                return cur_dts_avg +
                       ((new_layer_dts - last_layer_dts) / core::Rational(layer_len, 1));
            }

            static core::Rational update_dts_src(const TActiveLayers& active_layers) {
                core::Rational dts_src = active_layers[0]->dts();
                for (const auto& layer : active_layers) {
                    if (layer->dts() < dts_src) {
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
                              const core::VideoDecodeMethod& decode_method,
                              const size_t video_max_queue_count) {
            m_done_init = true;
            m_atom_profile = atom_profile;
            m_global_duration = global_duration;
            m_decode_method = decode_method;
            m_video_max_queue_count = video_max_queue_count;
            m_dts_src = decode_start;
            m_dts_dest = (std::min)(m_dts_src + this->BLOCK_SIZE, m_global_duration);

            for (size_t i = 0; i < atom_profile.layers.size(); i++) {
                m_layer_sources.push_back(make_owned<FFLayerSource>());
            }

            if (!this->collect_active_layers()) {
                // [TODO] sane solution?
                throw std::runtime_error("Not found active layers");
            }
            // m_dts_avg = detail::calc_dts_avg(m_active_layers);
            m_current_active_layer_idx = 0;
        }

        DecodeResult AtomSource::decode(const DecodeArg& decode_arg) {
            DecodeResult decode_result;
            decode_result.atom_uuid = m_atom_profile.uuid.c_str();

            if (m_current_active_layer_idx < m_active_layers.size()) {
                auto& cur_layer_source = m_active_layers[m_current_active_layer_idx];

                // [TODO] temporary disable balancing
                // const auto layer_too_early =
                //     (cur_layer_source->dts() - m_dts_avg) > core::Rational(500, 1000);
                const auto layer_too_early = false;
                if (layer_too_early || !cur_layer_source->can_decode()) {
                    m_current_active_layer_idx += 1;
                    return this->decode(decode_arg);
                }

                // const auto last_layer_dts = cur_layer_source->dts();
                decode_result = cur_layer_source->decode(decode_arg);
                // m_dts_avg = detail::update_dts_avg(m_dts_avg, m_active_layers.size(),
                //                                    last_layer_dts, cur_layer_source->dts());
                m_current_active_layer_idx += 1;
                return decode_result;
            }

            if (!this->all_active_layers_dead()) {
                m_dts_src = detail::update_dts_src(m_active_layers);
            }

            auto atom_eof = false;
            if (this->all_active_layers_dead() ||
                (m_dts_dest - m_dts_src) < core::Rational(10, 1000)) {
                atom_eof = !this->collect_active_layers();
            }

            if (atom_eof) {
                m_can_decode = false;
                decode_result.result = DecodeResultCode::DECODE_ATOM_ENDED;
                return decode_result;
            } else {
                // m_dts_avg = detail::calc_dts_avg(m_active_layers);
                m_current_active_layer_idx = 0;
                return this->decode(decode_arg);
            }
        }

        bool AtomSource::all_active_layers_dead(void) {
            for (const auto& layer_source : m_active_layers) {
                if (layer_source->can_decode()) {
                    return false;
                }
            }
            return true;
        }

        bool AtomSource::collect_active_layers(void) {
            m_active_layers.clear();
            m_current_active_layer_idx = 0;

            for (size_t i = 0; i < m_atom_profile.layers.size(); i++) {
                const auto& layer_prof = m_atom_profile.layers[i];
                auto within_block = m_dts_src <= layer_prof.to && layer_prof.from <= m_dts_dest;
                if (!within_block) {
                    continue;
                }

                if (!m_layer_sources[i]->done_init()) {
                    m_layer_sources[i]->init(m_atom_profile.layers[i], m_dts_src, m_decode_method,
                                             m_video_max_queue_count);
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
                    return this->collect_active_layers();
                }
            }

            m_dts_dest = (std::min)(m_dts_src + this->BLOCK_SIZE, m_global_duration);
            return true;
        }

    }

}
