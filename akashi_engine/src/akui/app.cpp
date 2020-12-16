#include "./app.h"

#include "./window.h"
#include "./interface/asp.h"
#include "./utils/widget.h"

#include <libakserver/akserver.h>
#include <libakstate/akstate.h>
#include <libakcore/memory.h>
#include <libakcore/logger.h>
#include <libakcore/config.h>

#include <QApplication>
#include <QTextCodec>
#include <thread>

#include <csignal>
#include <unistd.h>

using namespace akashi::core;
using namespace akashi::server;

namespace akashi {
    namespace ui {

        void UILoop::ui_thread(UILoopContext ctx, UILoop* loop) {
            QApplication app(ctx.argc, ctx.argv);

            QApplication::setApplicationName("Akashi Player");
            QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
            QTextCodec::setCodecForLocale(QTextCodec::codecForLocale());

            loop->set_on_thread_exit([](void*) { qApp->exit(0); }, nullptr);

            QObject::connect(&app, &QApplication::aboutToQuit, []() {
                // send SIGTERM to the main thread
                kill(getpid(), SIGTERM);
            });

            auto akconf = core::parse_akconfig(ctx.argv[1]);
            akashi::state::AKState state(akconf);

            Window window{borrowed_ptr(&state)};
            window.resize(akconf.ui.resolution.first, akconf.ui.resolution.second);
            window.show();

#ifndef NDEBUG
            walk_widgets(&window, ensure_widget_name);
#endif

            ASPAPISet api_set;
            api_set.general = new ASPGeneralAPIImpl(&window);
            api_set.media = new ASPMediaAPIImpl(&window);
            api_set.gui = new ASPGUIAPIImpl(&window);
            std::thread asp_thread(init_asp_server, (ASPConfig){"localhost", 1234},
                                   std::move(api_set));
            // [TODO] need to kill this thread properly
            asp_thread.detach();

            app.exec();

            AKLOG_INFON("UILoop::ui_thread(): Successfully exited");
        };

    }
}
