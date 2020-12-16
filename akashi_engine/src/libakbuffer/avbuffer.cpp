#include "./avbuffer.h"
#include "./video_queue.h"
#include "./audio_queue.h"

#include <libakcore/memory.h>
#include <libakstate/akstate.h>

using namespace akashi::core;

namespace akashi {

    namespace buffer {

        AVBuffer::AVBuffer(core::borrowed_ptr<state::AKState> state) {
            vq = make_owned<VideoQueue>(state);
            aq = make_owned<AudioQueue>(state);
        }

    }
}
