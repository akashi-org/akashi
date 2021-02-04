#include "akui/app.h"

#include <libakcore/logger.h>

#include <libakcore/config.h>
#include <libakcore/memory.h>
#include <libakstate/akstate.h>
#include <libakencoder/akencoder.h>
#include <libakencoder/encode_state.h>

#include <string>
#include <signal.h>
#include <string.h>

using namespace akashi::core;

void do_sigwait(sigset_t& ss) {
    int signum;
    sigwait(&ss, &signum);
    fprintf(stderr, "signal %s(%d) trapped\n", strsignal(signum), signum);
}

int main(int argc, char** argv) {
    sigset_t ss;

    sigemptyset(&ss);
    // sigfillset(&ss);
    // sigaddset(&ss, SIGINT);
    sigaddset(&ss, SIGHUP);
    sigaddset(&ss, SIGQUIT);
    sigaddset(&ss, SIGTERM);

    sigaddset(&ss, SIGPIPE);

    sigprocmask(SIG_BLOCK, &ss, NULL);

    if (argc < 2) {
        fprintf(stderr, "You must provide at least one argument\n");
        return 1;
    }

    LogCapabilities cap;

    if (std::getenv("AK_LOGLEVEL")) {
        cap.console_log_level =
            static_cast<LogLevel>(std::stoi(std::getenv("AK_LOGLEVEL"), nullptr));
    }
    if (std::getenv("AK_LOGFPATH")) {
        cap.log_fpath = std::getenv("AK_LOGFPATH");
    }

    create_logger(cap);

    if (std::getenv("AK_ENABLE_ENCODER")) {
        // [TODO] impl below
        // auto akconf = akashi::core::parse_akencoder_config(argv[1]);
        // akashi::encoder::EncodeState state(akconf);
        akashi::encoder::EncodeState state;
        akashi::encoder::EncodeLoop encode_loop;
        encode_loop.run({akashi::core::borrowed_ptr(&state)});

        do_sigwait(ss);
        encode_loop.terminate();
    } else {
        akashi::ui::UILoop ui_loop;
        ui_loop.run({argc, argv});

        do_sigwait(ss);
        ui_loop.terminate();
    }

    destroy_logger();
    return 0;
}
