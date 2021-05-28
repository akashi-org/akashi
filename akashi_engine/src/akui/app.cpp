#include "./app.h"

#include "./window.h"
#include "./interface/asp.h"
#include "./utils/widget.h"
#include "./utils/xutils.h"

#include <libakserver/akserver.h>
#include <libakstate/akstate.h>
#include <libakcore/memory.h>
#include <libakcore/logger.h>
#include <libakcore/config.h>

#include <QApplication>
#include <QTextCodec>
#include <thread>
#include <QSurfaceFormat>
#include <QScreen>
#include <QFontDatabase>

#include <QTimer>

#include <csignal>
#include <unistd.h>

using namespace akashi::core;
using namespace akashi::server;

namespace akashi {
    namespace ui {

        void UILoop::ui_thread(UILoopContext ctx, UILoop* loop) {
            QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

            QApplication app(ctx.argc, ctx.argv);

            QSurfaceFormat format;
            format.setVersion(4, 2);
            format.setProfile(QSurfaceFormat::CoreProfile);
            QSurfaceFormat::setDefaultFormat(format);

            QApplication::setApplicationName("Akashi Player");
            QTextCodec::setCodecForLocale(QTextCodec::codecForLocale());

            loop->set_on_thread_exit([](void*) { qApp->exit(0); }, nullptr);

            QObject::connect(&app, &QApplication::aboutToQuit, []() {
                // send SIGTERM to the main thread
                kill(getpid(), SIGTERM);
            });

            if (QFontDatabase::addApplicationFont(":/FontAwesome-solid.otf") < 0) {
                AKLOG_ERRORN("Failed to load font: FontAwesome-solid.otf\n");
            }

            auto akconf = core::parse_akconfig(ctx.argv[1]);
            akashi::state::AKState state(akconf, ctx.argv[2]);

            Window window{borrowed_ptr(&state)};
            // disable auto focus on startup
            if (state.m_ui_conf.window_mode != core::WindowMode::INDEPENDENT) {
                window.setAttribute(Qt::WA_ShowWithoutActivating);
                window.setWindowFlags(window.windowFlags() | Qt::FramelessWindowHint);
            }

            if (state.m_ui_conf.window_mode != core::WindowMode::IMMERSIVE) {
                window.resize(akconf.ui.resolution.first, akconf.ui.resolution.second);
                auto screen_geom = QApplication::primaryScreen()->geometry();
                auto padding = screen_geom.height() * 0.02;
                window.move((screen_geom.width() - akconf.ui.resolution.first) - padding, padding);
                window.show();
            } else {
                window.showFullScreen();
            }

            if (state.m_ui_conf.window_mode != core::WindowMode::INDEPENDENT) {
                auto disp = get_x_display();
                auto parent_win = get_current_active_window(disp);
                if (parent_win) {
                    // call it after window.show()
                    set_transient(disp, parent_win, &window);
                }

                // free_x_display_wrapper(disp);
                // free_x_window_wrapper(parent_win);
                QObject::connect(&window, &Window::window_activated, [disp, parent_win]() {
                    QSharedPointer<QTimer> timer = QSharedPointer<QTimer>::create();
                    QSharedPointer<int> rest_count = QSharedPointer<int>::create();
                    *rest_count = 10;
                    timer->setInterval(100);
                    QObject::connect(
                        timer.data(), &QTimer::timeout, [rest_count, timer, disp, parent_win]() {
                            if (is_active_window(disp, parent_win)) {
                                AKLOG_INFON("Editor window successfully raised!");
                                timer->stop();
                            }
                            if (*rest_count < 1) {
                                AKLOG_ERRORN("TIMEOUT");
                                // [TODO] need clear()?
                                timer->stop();
                            } else {
                                raise_window(disp, parent_win);
                                (*rest_count)--;
                                AKLOG_INFO("...raising editor window: {}", *rest_count);
                            }
                        });
                    raise_window(disp, parent_win);
                    timer->start();
                });
            }

#ifndef NDEBUG
            walk_widgets(&window, ensure_widget_name);
#endif

            ASPAPISet api_set;
            api_set.general = new ASPGeneralAPIImpl(&window);
            api_set.media = new ASPMediaAPIImpl(&window);
            api_set.gui = new ASPGUIAPIImpl(&window);
            std::thread asp_thread(init_renderer_asp_server, std::move(api_set));
            // [TODO] need to kill this thread properly
            asp_thread.detach();

            app.exec();

            AKLOG_INFON("UILoop::ui_thread(): Successfully exited");
        };

    }
}
