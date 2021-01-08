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

#include <vector>

using namespace akashi::core;

namespace akashi {
    namespace ui {

        static void* get_proc_address(void*, const char* name) {
            QOpenGLContext* glctx = QOpenGLContext::currentContext();
            if (!glctx) {
                return nullptr;
            }
            return reinterpret_cast<void*>(glctx->getProcAddress(QByteArray(name)));
        }

        PlayerWidget::PlayerWidget(akashi::core::borrowed_ptr<akashi::state::AKState> state,
                                   QWidget* parent, Qt::WindowFlags f)
            : QOpenGLWidget(parent, f), m_state(state) {
            m_player = make_owned<akashi::player::AKPlayer>(m_state);
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
            if (m_player->evalbuf_dequeue_ready()) {
                m_player->play();
            } else {
                AKLOG_WARNN("Not ready for playing");
            }
        };

        void PlayerWidget::pause(void) { m_player->pause(); }

        void PlayerWidget::seek(const akashi::core::Rational& pos) { m_player->seek(pos); }

        void PlayerWidget::relative_seek(const akashi::core::Rational& rel_pos) {
            m_player->relative_seek(rel_pos);
        }

        void PlayerWidget::frame_step(void) { m_player->frame_step(); }

        void PlayerWidget::frame_back_step(void) { m_player->frame_back_step(); }

        void PlayerWidget::initializeGL() {
            m_player->init({PlayerWidget::on_event}, this, {get_proc_address});
        }

        void PlayerWidget::paintGL() {
            akashi::graphics::RenderParams params;
            params.screen_width = this->width();
            params.screen_height = this->height();
            params.default_fb = this->defaultFramebufferObject();
            m_player->render(params);
        }

        void PlayerWidget::mouseMoveEvent(QMouseEvent*) {
            if (m_enable_smart_cursor) {
                QApplication::restoreOverrideCursor();
                if (m_cursor_timer) {
                    m_cursor_timer->start();
                }
            }
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
                    Fraction frac;
                    frac.num = evt.int64_value;
                    frac.den = evt.int64_value2;
                    Q_EMIT this->time_changed(frac);
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
