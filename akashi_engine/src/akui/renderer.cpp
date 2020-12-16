#include "akui/app.h"

#include <libakcore/logger.h>

#include <string>
#include <signal.h>
#include <string.h>

using namespace akashi::core;

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

    akashi::ui::UILoop ui_loop;
    ui_loop.run({argc, argv});

    int signum;
    sigwait(&ss, &signum);

    fprintf(stderr, "signal %s(%d) trapped\n", strsignal(signum), signum);
    ui_loop.terminate();

    destroy_logger();
    return 0;
}
