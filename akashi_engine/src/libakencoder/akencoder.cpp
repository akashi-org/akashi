#include "./akencoder.h"

#include <libakcore/logger.h>

#include <csignal>
#include <unistd.h>

// temporary
#include <chrono>
#include <thread>

using namespace akashi::core;

namespace akashi {
    namespace encoder {

        void EncodeLoop::encode_thread(EncodeLoopContext, EncodeLoop* loop) {
            AKLOG_INFON("Encoder init");

            loop->set_on_thread_exit([](void*) { AKLOG_INFON("Successfully exited"); }, nullptr);

            int wait_millsec = 5000;
            while (wait_millsec > 0) {
                AKLOG_INFON("...now encoding");
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                wait_millsec -= 1000;
            }
            // send SIGTERM to the main thread
            kill(getpid(), SIGTERM);
        }

    }
}
