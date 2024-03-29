#include "./PlayerWidget.h"

#include <libakcore/error.h>
#include <libakcore/element.h>
#include <libakcore/memory.h>
#include <libakcore/logger.h>
#include <libakplayer/akplayer.h>
#include <libakevent/akevent.h>
#include <libakgraphics/item.h>

#include <QOpenGLContext>
#include <QApplication>
#include <QTimer>
#include <QMouseEvent>

#include <EGL/egl.h>

#include <vector>

using namespace akashi::core;

namespace akashi {
    namespace ui {

        static void* get_proc_address(const char* name) {
            QOpenGLContext* glctx = QOpenGLContext::currentContext();
            if (!glctx) {
                return nullptr;
            }
            return reinterpret_cast<void*>(glctx->getProcAddress(QByteArray(name)));
        }

        static void* egl_get_proc_address(const char* name) {
            // [TODO] add checks for validity of egl?
            return reinterpret_cast<void*>(eglGetProcAddress(name));
        }

        PlayerWidget::PlayerWidget(akashi::core::borrowed_ptr<akashi::state::AKState> state,
                                   akashi::core::borrowed_ptr<akashi::player::AKPlayer> player,
                                   QWidget* parent, Qt::WindowFlags f)
            : QOpenGLWidget(parent, f), m_state(state), m_player(player) {
            this->setObjectName("player_widget");

            this->setMouseTracking(true);
            m_cursor_timer = new QTimer(this);
            m_cursor_timer->setInterval(PlayerWidget::CURSOR_INTERVAL_MS);
            m_cursor_timer->setSingleShot(true);
            QObject::connect(m_cursor_timer, &QTimer::timeout, [this]() {
                if (m_enable_smart_cursor) {
                    QApplication::setOverrideCursor(QCursor(Qt::BlankCursor));
                }
            });
        }

        PlayerWidget::~PlayerWidget() {}

        void PlayerWidget::play(void) {
            if (m_player->kron_ready()) {
                m_player->play();
            } else {
                AKLOG_WARNN("Not ready for playing");
            }
        };

        void PlayerWidget::pause(void) { m_player->pause(); }

        void PlayerWidget::seek(const akashi::core::Rational& pos) { m_player->seek(pos); }

        void PlayerWidget::relative_seek(const double ratio) { m_player->relative_seek(ratio); }

        void PlayerWidget::frame_seek(int nframes) { m_player->frame_seek(nframes); }

        void PlayerWidget::frame_step(void) { m_player->frame_step(); }

        void PlayerWidget::frame_back_step(void) { m_player->frame_back_step(); }

        core::Rational PlayerWidget::current_time(void) { return m_player->current_frame_time(); }

        void PlayerWidget::inline_eval(const std::string& file_path, const std::string& elem_name) {
            m_player->inline_eval(file_path, elem_name);
        }

        void PlayerWidget::set_volume(const double volume) { m_player->set_volume(volume); }

        void PlayerWidget::initializeGL() {
            m_player->init({PlayerWidget::on_event}, this, {get_proc_address},
                           {egl_get_proc_address});
        }

        void PlayerWidget::paintGL() {
            akashi::graphics::RenderParams params;
            params.screen_width = this->width();
            params.screen_height = this->height();
            params.default_fb = this->defaultFramebufferObject();
            m_player->render(params);
        }

        void PlayerWidget::mouseMoveEvent(QMouseEvent* event) {
            if (m_enable_smart_cursor) {
                QApplication::restoreOverrideCursor();
                if (m_cursor_timer) {
                    m_cursor_timer->start();
                }
            }
            event->ignore();
        }

        void PlayerWidget::enterEvent(QEvent*) { m_enable_smart_cursor = true; }

        void PlayerWidget::leaveEvent(QEvent*) {
            m_enable_smart_cursor = false;
            QApplication::restoreOverrideCursor();
        }

        void PlayerWidget::on_event(void* evt_ctx, akashi::event::Event evt) {
            if (evt_ctx) {
                // [TODO] is it ok with queuedconnection?
                QMetaObject::invokeMethod(
                    (PlayerWidget*)evt_ctx,
                    // [XXX] does not work properly unless evt_ctx, evt is fully copied
                    [evt_ctx, evt]() {
                        static_cast<PlayerWidget*>(evt_ctx)->handle_player_event(evt);
                    },
                    Qt::QueuedConnection);
            }
        }

        // [TODO] can it be possible to remove seemingly-unnecessary copy?
        void PlayerWidget::handle_player_event(akashi::event::Event evt) {
            using EventName = akashi::event::EventName;

            switch (evt.name) {
                case EventName::TIME_UPDATE: {
                    core::Rational rat{evt.int64_value, evt.int64_value2};
                    Q_EMIT this->time_changed(rat);
                    break;
                }
                case EventName::SET_RENDER_PROF: {
                    m_player->set_render_prof(evt.render_prof);
                    Q_EMIT this->render_prof_updated(evt.render_prof);
                    break;
                }
                case EventName::UPDATE: {
                    this->update();
                    break;
                }
                case EventName::CHANGE_PLAY_STATE: {
                    Q_EMIT this->play_state_changed(evt.play_state);
                    break;
                }
                case EventName::SEEK_COMPLETED: {
                    Q_EMIT this->seek_completed();
                    break;
                }
                default: {
                    AKLOG_ERROR("Invalid event emitted. EventID: {}", static_cast<int>(evt.name));
                }
            }
        }

    }
}
