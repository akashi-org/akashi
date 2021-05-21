#include "./window.h"

#include "./container/ControlArea.h"
#include "./container/MonitorArea.h"

#include <libakcore/rational.h>
#include <libakcore/element.h>
#include <libakcore/logger.h>
#include <libakcore/memory.h>
#include <libakstate/akstate.h>

#include <QApplication>
#include <QGridLayout>
#include <QWidget>
#include <QMouseEvent>
#include <QResizeEvent>

#include <QSizeGrip>

using namespace akashi::core;

namespace akashi {
    namespace ui {

        ExitButton::ExitButton(QWidget* parent, Qt::WindowFlags f) : QLabel(parent, f) {
            this->setObjectName("exit_btn");

            QFont font;
            font.setPointSize(24);
            this->setFont(font);

            m_base_css = "margin-left: 10px; margin-top: 5px;";
            this->setStyleSheet((m_base_css + "color: transparent").c_str());
            this->setText("\u00d7");
        }

        ExitButton::~ExitButton() = default;

        void ExitButton::mouseReleaseEvent(QMouseEvent*) { QApplication::quit(); }

        void ExitButton::enterEvent(QEvent*) {
            this->setStyleSheet((m_base_css + "color: white").c_str());
        }

        void ExitButton::leaveEvent(QEvent*) {
            this->setStyleSheet((m_base_css + "color: transparent").c_str());
        }

        static bool can_seek(akashi::core::borrowed_ptr<akashi::state::AKState> state) {
            return state->get_seek_completed() && state->m_atomic_state.ui_can_seek.load();
        }

        Window::Window(akashi::core::borrowed_ptr<akashi::state::AKState> state, QWidget* parent)
            : QFrame(parent), m_state(state) {
            this->setObjectName("window");
            // this->setStyleSheet("#window {border: 1px solid #091F2B; }");
            // this->setFrameShadow(QFrame::Raised);

            this->m_monitorArea = new MonitorArea(borrowed_ptr(m_state), this);
            this->m_controlArea = new ControlArea(borrowed_ptr(m_state), this);

            switch (m_state->m_ui_conf.window_mode) {
                case core::WindowMode::SPLIT: {
                    this->setTransparent(true);
                    break;
                }
                case core::WindowMode::IMMERSIVE: {
                    this->setTransparent(true);
                    this->m_controlArea->hide();
                    break;
                }
                case core::WindowMode::INDEPENDENT: {
                    break;
                }
                default: {
                    break;
                }
            }

            m_sizeGrip = new QSizeGrip{this};
            m_sizeGrip->setObjectName("size_grip");
            m_sizeGrip->setStyleSheet("background: transparent");

            m_exitBtn = new ExitButton{this};

            this->m_mainLayout = new QGridLayout;
            this->m_mainLayout->setContentsMargins(0, 0, 0, 0);
            this->m_mainLayout->setSpacing(0);
            // [XXX] keep it align unset, or the expansion does not work properly
            this->m_mainLayout->addWidget(this->m_monitorArea, 0, 0, -1, -1);
            this->m_mainLayout->addWidget(this->m_controlArea, 0, 0, -1, -1, Qt::AlignBottom);

            this->m_mainLayout->addWidget(m_exitBtn, 0, 0, -1, -1, Qt::AlignTop | Qt::AlignLeft);
            this->m_mainLayout->addWidget(m_sizeGrip, 0, 0, -1, -1,
                                          Qt::AlignBottom | Qt::AlignRight);

            this->setLayout(this->m_mainLayout);
        }

        Window::~Window(){};

        void Window::toggleFullScreen(void) {
            switch (m_state->m_ui_conf.window_mode) {
                case core::WindowMode::SPLIT:
                case core::WindowMode::INDEPENDENT: {
                    if (this->isFullScreen()) {
                        this->showNormal();
                    } else {
                        this->showFullScreen();
                    }
                    break;
                }
                case core::WindowMode::IMMERSIVE: {
                    if (this->windowOpacity() < 1.0) {
                        this->m_controlArea->show();
                        this->setTransparent(false);
                    } else {
                        this->m_controlArea->hide();
                        this->setTransparent(true);
                    }
                    break;
                }
                default: {
                    break;
                }
            }
        }

        void Window::setTransparent(bool transparent) {
            if (transparent) {
                switch (m_state->m_ui_conf.window_mode) {
                    case core::WindowMode::SPLIT:
                    case core::WindowMode::INDEPENDENT: {
                        this->setWindowOpacity(0.75);
                        break;
                    }
                    case core::WindowMode::IMMERSIVE: {
                        this->setWindowOpacity(0.4);
                        break;
                    }
                    default: {
                        break;
                    }
                }
            } else {
                this->setWindowOpacity(1.0);
            }
        }

        void Window::changePlayState(const state::PlayState& new_state) {
            if (m_state->m_atomic_state.icon_play_state == new_state) {
                return;
            }
            switch (new_state) {
                case akashi::state::PlayState::STOPPED:
                case akashi::state::PlayState::PAUSED: {
                    m_state->m_atomic_state.icon_play_state.store(akashi::state::PlayState::PAUSED);
                    if (m_state->m_ui_conf.window_mode == core::WindowMode::IMMERSIVE) {
                        this->setTransparent(true);
                        this->m_controlArea->hide();
                    }
                    this->m_monitorArea->pause();
                    break;
                }
                case akashi::state::PlayState::PLAYING: {
                    if (!m_state->get_evalbuf_dequeue_ready()) {
                        AKLOG_WARNN("kron not ready");
                        return;
                    }

                    m_state->m_atomic_state.icon_play_state.store(
                        akashi::state::PlayState::PLAYING);

                    if (m_state->m_ui_conf.window_mode == core::WindowMode::IMMERSIVE) {
                        this->setTransparent(false);
                        this->m_controlArea->show();
                    }
                    this->m_monitorArea->play();
                    break;
                }
                default: {
                }
            }
            Q_EMIT this->state_changed(new_state);
        }

        void Window::showEvent(QShowEvent*) { m_origSize = this->size(); }

        void Window::changeEvent(QEvent* event) {
            if (event->type() == QEvent::ActivationChange) {
                if (this->isActiveWindow()) {
                    Q_EMIT this->window_activated();
                }
            }
            if (event->type() == QEvent::WindowStateChange) {
                if (this->isFullScreen()) {
                    if (m_state->m_ui_conf.window_mode == core::WindowMode::SPLIT) {
                        this->setTransparent(false);
                    }
                    Q_EMIT this->m_controlArea->hide_control();
                } else {
                    if (m_state->m_ui_conf.window_mode != core::WindowMode::INDEPENDENT) {
                        this->setTransparent(true);
                    }
                    Q_EMIT this->m_controlArea->show_control();
                }
            }
        }

        void Window::mousePressEvent(QMouseEvent* event) { m_lastMouse = event->pos(); }

        void Window::mouseReleaseEvent(QMouseEvent*) {
            // ensure the parent window to be raised
            Q_EMIT this->window_activated();
        }

        void Window::mouseMoveEvent(QMouseEvent* event) {
            this->move(this->pos() + (event->pos() - m_lastMouse));
        }

        void Window::mouseDoubleClickEvent(QMouseEvent*) {
            this->toggleFullScreen();
            // this->resize(this->m_origSize);
        }

        void Window::on_seek(const akashi::core::Rational& time) {
            if (can_seek(m_state)) {
                m_state->m_atomic_state.ui_can_seek.store(false);
                this->m_controlArea->set_slider_movable(false);
                this->m_monitorArea->seek(time);
                m_state->m_atomic_state.icon_play_state.store(akashi::state::PlayState::PAUSED);
                Q_EMIT this->state_changed(m_state->m_atomic_state.icon_play_state.load());
            }
        }

        void Window::on_frame_step(void) {
            if (can_seek(m_state)) {
                m_state->m_atomic_state.ui_can_seek.store(false);
                this->m_controlArea->set_slider_movable(false);
                this->m_monitorArea->frame_step();
                m_state->m_atomic_state.icon_play_state.store(akashi::state::PlayState::PAUSED);
                Q_EMIT this->state_changed(m_state->m_atomic_state.icon_play_state.load());
            }
        }

        void Window::on_frame_back_step(void) {
            if (can_seek(m_state)) {
                m_state->m_atomic_state.ui_can_seek.store(false);
                this->m_controlArea->set_slider_movable(false);
                this->m_monitorArea->frame_back_step();
                m_state->m_atomic_state.icon_play_state.store(akashi::state::PlayState::PAUSED);
                Q_EMIT this->state_changed(m_state->m_atomic_state.icon_play_state.load());
            }
        }

        void Window::on_forward_jump(const akashi::core::Rational& rel_pos) {
            if (can_seek(m_state)) {
                m_state->m_atomic_state.ui_can_seek.store(false);
                this->m_controlArea->set_slider_movable(false);
                this->m_monitorArea->relative_seek(rel_pos);
                m_state->m_atomic_state.icon_play_state.store(akashi::state::PlayState::PAUSED);
                Q_EMIT this->state_changed(m_state->m_atomic_state.icon_play_state.load());
            }
        }

        void Window::on_backward_jump(const akashi::core::Rational& rel_pos) {
            if (can_seek(m_state)) {
                m_state->m_atomic_state.ui_can_seek.store(false);
                this->m_controlArea->set_slider_movable(false);
                this->m_monitorArea->relative_seek(Rational(-1, 1) * rel_pos);
                m_state->m_atomic_state.icon_play_state.store(akashi::state::PlayState::PAUSED);
                Q_EMIT this->state_changed(m_state->m_atomic_state.icon_play_state.load());
            }
        }

        void Window::on_state_change(const akashi::state::PlayState& new_state) {
            switch (new_state) {
                case akashi::state::PlayState::STOPPED:
                case akashi::state::PlayState::PAUSED: {
                    this->m_monitorArea->pause();
                    break;
                }
                case akashi::state::PlayState::PLAYING: {
                    if (!m_state->get_evalbuf_dequeue_ready()) {
                        AKLOG_WARNN("Window::on_state_change(): kron not ready");
                        return;
                    }
                    this->m_monitorArea->play();
                    break;
                }
                default: {
                }
            }
            Q_EMIT this->state_changed(new_state);
        }

        void Window::on_state_toggle(void) {
            switch (m_state->m_atomic_state.icon_play_state.load()) {
                case akashi::state::PlayState::STOPPED:
                case akashi::state::PlayState::PAUSED: {
                    this->changePlayState(akashi::state::PlayState::PLAYING);
                    break;
                }
                case akashi::state::PlayState::PLAYING: {
                    this->changePlayState(akashi::state::PlayState::PAUSED);
                    break;
                }
                default: {
                }
            }
        }

        void Window::on_state_update(const akashi::state::PlayState& new_state) {
            this->changePlayState(new_state);
        }

        void Window::on_time_change(akashi::core::Fraction& time_frac) {
            Q_EMIT this->time_changed(to_rational(time_frac));
        }

        void Window::on_render_prof_updated(akashi::core::RenderProfile& render_prof) {
            Q_EMIT this->render_prof_changed(render_prof);
        }

        void Window::on_seek_completed(void) {
            m_state->m_atomic_state.ui_can_seek.store(true);
            this->m_controlArea->set_slider_movable(true);
        }
    }
}
