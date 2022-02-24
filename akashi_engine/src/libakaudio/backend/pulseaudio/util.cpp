#include "./util.h"
#include "./etc.h"

#include <libakcore/logger.h>
#include <libakcore/audio.h>

#include <pulse/pulseaudio.h>

using namespace akashi::core;

namespace akashi {
    namespace audio {

        MainloopLockGuard::MainloopLockGuard(pa_threaded_mainloop* mainloop)
            : m_mainloop(mainloop) {
            pa_threaded_mainloop_lock(m_mainloop);
        }
        MainloopLockGuard::~MainloopLockGuard() { pa_threaded_mainloop_unlock(m_mainloop); }

        // [TODO] needs pa_threaded_mainloop_lock()?
        void context_inspect(pa_context* context) {
            pa_context_get_source_info_list(
                context,
                [](pa_context*, const pa_source_info* i, int eol, void*) {
                    if (eol) {
                        return;
                    }
                    AKLOG_INFO("source: idx: {}, name: {}", i->index, i->name);
                },
                nullptr);

            pa_context_get_sink_info_list(
                context,
                [](pa_context*, const pa_sink_info* i, int eol, void*) {
                    if (eol) {
                        return;
                    }
                    AKLOG_INFO("sink: idx: {}, name: {}", i->index, i->name);
                },
                nullptr);
        };

        PASampleFormat to_pl_sample_format(const core::AKAudioSampleFormat& format) {
            // [TODO] might not work properly in a big endian system like Windows
            switch (format) {
                case AKAudioSampleFormat::U8:
                    return {PA_SAMPLE_U8};
                case AKAudioSampleFormat::S16:
                    return {PA_SAMPLE_S16LE};
                case AKAudioSampleFormat::S32:
                    return {PA_SAMPLE_S32LE};
                case AKAudioSampleFormat::FLT:
                    return {PA_SAMPLE_FLOAT32LE};
                default:
                    AKLOG_ERROR("to_pl_sample_format() failed. Invalid format {}", format);
                    return {PA_SAMPLE_INVALID};
            }
        }

    }
}
