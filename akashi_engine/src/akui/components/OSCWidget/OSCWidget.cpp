#include "./OSCWidget.h"
#include "../../window.h"

#include <libakcore/error.h>
#include <libakcore/element.h>
#include <libakcore/memory.h>
#include <libakcore/logger.h>
#include <libakevent/akevent.h>
#include <libakgraphics/item.h>
#include <libakgraphics/osc.h>

#include <QOpenGLContext>
#include <QApplication>
#include <QTimer>
#include <QMouseEvent>
#include <QCursor>

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

        OSCWidget::OSCWidget(akashi::core::borrowed_ptr<akashi::state::AKState> state,
                             QWidget* parent, Qt::WindowFlags f)
            : QOpenGLWidget(parent, f) {
            m_osc = make_owned<akashi::graphics::OSCWidget>(state);
            this->setObjectName("osc_widget");
            // [XXX] a hack for the transparency issues
            // @ref: https://stackoverflow.com/questions/32286329/qopenglwidget-and-transparency
            this->setAttribute(Qt::WA_AlwaysStackOnTop);

            auto [init_w, init_h] = state->m_ui_conf.resolution;
            this->setMinimumSize(init_w * 0.8, init_h * 0.4);

            this->setMouseTracking(true);
            m_cursor_timer = new QTimer(this);
            m_cursor_timer->setInterval(OSCWidget::CURSOR_INTERVAL_MS);
            m_cursor_timer->setSingleShot(true);
            QObject::connect(m_cursor_timer, &QTimer::timeout, [this]() {
                if (m_enable_smart_cursor) {
                    QApplication::setOverrideCursor(QCursor(Qt::BlankCursor));
                }
            });

            m_mouse_hold_timer = new QTimer(this);
            m_mouse_hold_timer->setInterval(OSCWidget::MOUSE_HOLD_INTERVAL_MS);
            QObject::connect(m_mouse_hold_timer, &QTimer::timeout, [this]() {
                if (m_last_pressed_btn != Qt::NoButton) {
                    auto qmouse_pos = this->mapFromGlobal(QCursor::pos());
                    using namespace akashi::graphics;
                    m_osc->emit_mouse_event(
                        {.kind = akashi::graphics::OSCMouseEventKind::HOLD,
                         .pos = {qmouse_pos.x(), qmouse_pos.y()},
                         .btn = m_last_pressed_btn == Qt::LeftButton     ? OSCMouseButton::LEFT
                                : m_last_pressed_btn == Qt::MiddleButton ? OSCMouseButton::MIDDLE
                                : m_last_pressed_btn == Qt::RightButton  ? OSCMouseButton::RIGHT
                                                                         : OSCMouseButton::LENGTH});
                } else {
                    m_mouse_hold_timer->stop();
                }
            });
        }

        OSCWidget::~OSCWidget() {}

        void OSCWidget::initializeGL() {
            m_osc->load_api({OSCWidget::on_event, this}, this->current_render_params(),
                            {get_proc_address}, {egl_get_proc_address});
        }

        void OSCWidget::paintGL() { m_osc->render(this->current_render_params()); }

        void OSCWidget::resizeGL(int /*w*/, int /*h*/) {
            m_osc->resize(this->current_render_params());
        }

        void OSCWidget::update_current_time(const akashi::core::Rational& current_time) {
            using namespace akashi::graphics;
            auto should_update = m_osc->emit_time_event(
                {.kind = OSCTimeEventKind::CURRENT_TIME, .current_time = current_time});
            if (should_update) {
                this->update();
            }
        }

        void OSCWidget::update_duration(const akashi::core::RenderProfile& render_prof) {
            using namespace akashi::graphics;
            auto should_update = m_osc->emit_time_event(
                {.kind = OSCTimeEventKind::DURATION, .duration = render_prof.duration});
            if (should_update) {
                this->update();
            }
        }

        void OSCWidget::mousePressEvent(QMouseEvent* event) {
            auto qmouse_pos = this->mapFromGlobal(QCursor::pos());
            m_last_pressed_btn = event->button();
            m_mouse_hold_timer->start();

            using namespace akashi::graphics;
            auto is_accepted = m_osc->emit_mouse_event(
                {.kind = akashi::graphics::OSCMouseEventKind::PRESS,
                 .pos = {qmouse_pos.x(), qmouse_pos.y()},
                 .btn = event->buttons() & Qt::LeftButton     ? OSCMouseButton::LEFT
                        : event->buttons() & Qt::MiddleButton ? OSCMouseButton::MIDDLE
                        : event->buttons() & Qt::RightButton  ? OSCMouseButton::RIGHT
                                                              : OSCMouseButton::LENGTH});
            if (is_accepted) {
                // for smooth seeking
                this->grabMouse();
            }

            event->setAccepted(is_accepted);
        }

        void OSCWidget::mouseMoveEvent(QMouseEvent* event) {
            auto qmouse_pos = this->mapFromGlobal(QCursor::pos());

            using namespace akashi::graphics;
            auto is_accepted = m_osc->emit_mouse_event(
                {.kind = akashi::graphics::OSCMouseEventKind::MOVE,
                 .pos = {qmouse_pos.x(), qmouse_pos.y()},
                 .btn = event->buttons() & Qt::LeftButton     ? OSCMouseButton::LEFT
                        : event->buttons() & Qt::MiddleButton ? OSCMouseButton::MIDDLE
                        : event->buttons() & Qt::RightButton  ? OSCMouseButton::RIGHT
                                                              : OSCMouseButton::LENGTH});

            if (m_enable_smart_cursor) {
                QApplication::restoreOverrideCursor();
                if (m_cursor_timer) {
                    m_cursor_timer->start();
                }
            }

            event->setAccepted(is_accepted);
        }

        void OSCWidget::mouseReleaseEvent(QMouseEvent* event) {
            auto qmouse_pos = this->mapFromGlobal(QCursor::pos());
            m_last_pressed_btn = Qt::NoButton;

            using namespace akashi::graphics;
            auto is_accepted = m_osc->emit_mouse_event(
                {.kind = akashi::graphics::OSCMouseEventKind::RELEASE,
                 .pos = {qmouse_pos.x(), qmouse_pos.y()},
                 .btn = event->buttons() & Qt::LeftButton     ? OSCMouseButton::LEFT
                        : event->buttons() & Qt::MiddleButton ? OSCMouseButton::MIDDLE
                        : event->buttons() & Qt::RightButton  ? OSCMouseButton::RIGHT
                                                              : OSCMouseButton::LENGTH});

            this->releaseMouse();

            event->setAccepted(is_accepted);
        }

        void OSCWidget::enterEvent(QEvent*) { m_enable_smart_cursor = true; }

        void OSCWidget::leaveEvent(QEvent*) {
            m_enable_smart_cursor = false;
            QApplication::restoreOverrideCursor();
        }

        akashi::graphics::RenderParams OSCWidget::current_render_params() {
            akashi::graphics::RenderParams params;
            params.screen_width = this->width();
            params.screen_height = this->height();
            params.default_fb = this->defaultFramebufferObject();
            auto qmouse_pos = this->mapFromGlobal(QCursor::pos());
            params.mouse_pos = {qmouse_pos.x(), qmouse_pos.y()};

            return params;
        }

        void OSCWidget::on_event(void* evt_ctx, akashi::graphics::OSCInnerEvent evt) {
            if (evt_ctx) {
                // [TODO] is it ok with queuedconnection?
                QMetaObject::invokeMethod(
                    (OSCWidget*)evt_ctx,
                    // [XXX] does not work properly unless evt_ctx, evt is fully copied
                    [evt_ctx, evt]() { static_cast<OSCWidget*>(evt_ctx)->handle_osc_event(evt); },
                    Qt::QueuedConnection);
            }
        }

        void OSCWidget::handle_osc_event(akashi::graphics::OSCInnerEvent evt) {
            using namespace akashi::graphics;
            switch (evt.name) {
                case OSCInnerEventName::PLAYBTN_CLICKED: {
                    Q_EMIT this->playBtn_clicked();
                    break;
                }
                case OSCInnerEventName::VOLUME_CHANGED: {
                    auto gain = reinterpret_cast<double*>(evt.args.get());
                    Q_EMIT this->volume_changed(*gain);
                    break;
                }
                case OSCInnerEventName::UPDATE: {
                    this->update();
                    break;
                }
                case OSCInnerEventName::FRAME_SEEK: {
                    auto nframes = reinterpret_cast<int*>(evt.args.get());
                    Q_EMIT this->frame_seek(*nframes);
                    break;
                }
                case OSCInnerEventName::SEEKBAR_PRESSED: {
                    auto seek_value = reinterpret_cast<core::Rational*>(evt.args.get());
                    Q_EMIT this->seekbar_pressed(*seek_value);
                    break;
                }
                case OSCInnerEventName::SEEKBAR_MOVED: {
                    auto seek_value = reinterpret_cast<core::Rational*>(evt.args.get());
                    Q_EMIT this->seekbar_moved(*seek_value);
                    break;
                }
                case OSCInnerEventName::SEEKBAR_RELEASED: {
                    auto seek_value = reinterpret_cast<core::Rational*>(evt.args.get());
                    Q_EMIT this->seekbar_released(*seek_value);
                    break;
                }
                default: {
                    AKLOG_ERROR("Invalid event emitted. EventID: {}", static_cast<int>(evt.name));
                }
            }
        }

    }
}
