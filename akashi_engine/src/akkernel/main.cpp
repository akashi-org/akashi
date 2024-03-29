#include "./kernel.h"

#include <version.h>

#include <libakcore/logger.h>

#include <libakcore/config.h>
#include <libakcore/memory.h>
#include <libakstate/akstate.h>

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
    // sigaddset(&ss, SIGCHLD);
    // sigaddset(&ss, SIGSEGV);

    sigaddset(&ss, SIGPIPE);

    sigprocmask(SIG_BLOCK, &ss, NULL);

    if (argc < 2) {
        fprintf(stderr, "You must provide at least one argument\n");
        return 1;
    }

    if (std::string(argv[1]) == "--version") {
        fprintf(stderr, "akashi_kernel %d.%d-%s\n", AKASHI_ENGINE_VERSION_MAJOR,
                AKASHI_ENGINE_VERSION_MINOR, AKASHI_GIT_REV);
        return 0;
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

    auto akconf = akashi::core::parse_akconfig(argv[1]);
    akashi::state::AKState state(akconf, argv[3]);

    AKLOG_INFON("Akashi Kernel Init");

    akashi::kernel::KernelLoop kernel_loop;
    kernel_loop.run({argv[1], argv[2], static_cast<uint32_t>(std::stol(argv[4])), argv[3]});

    int signum;
    sigwait(&ss, &signum);
    AKLOG_INFO("signal {}({}) trapped", strsignal(signum), signum);

    kernel_loop.terminate();

    AKLOG_INFON("Akashi Kernel Terminate");

    destroy_logger();
    return 0;
}
