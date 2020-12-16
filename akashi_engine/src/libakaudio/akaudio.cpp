#include "./akaudio.h"
#include "./context.h"
#include "./backend/pulseaudio.h"

#include <libakbuffer/avbuffer.h>
#include <libakbuffer/audio_queue.h>
#include <libakstate/akstate.h>
#include <libakcore/rational.h>
#include <libakcore/memory.h>
#include <libakcore/logger.h>

using namespace akashi::core;

namespace akashi {
    namespace audio {

        AKAudio::AKAudio(core::borrowed_ptr<state::AKState> state,
                         core::borrowed_ptr<buffer::AVBuffer> buffer,
                         core::borrowed_ptr<event::AKEvent> event) {
            m_audio_ctx = make_owned<PulseAudioContext>(state, buffer, event);
        }

        AKAudio::~AKAudio() {}

        void AKAudio::destroy(void) { m_audio_ctx->destroy(); }

        void AKAudio::play(void) { m_audio_ctx->play(); }

        void AKAudio::pause(void) { m_audio_ctx->pause(); }

        void AKAudio::stop(void) { m_audio_ctx->stop(); }

        core::Rational AKAudio::current_time(void) const { return m_audio_ctx->current_time(); }

    }
}
