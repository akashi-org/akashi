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

using namespace akashi::core;

namespace akashi {
    namespace ui {

        static bool can_seek(akashi::core::borrowed_ptr<akashi::state::AKState> state) {
            return state->get_seek_completed() && state->m_atomic_state.ui_can_seek.load();
        }

        Window::Window(akashi::core::borrowed_ptr<akashi::state::AKState> state, QWidget* parent)
            : QWidget(parent), m_state(state) {
            this->setObjectName("window");
            this->monitorArea = new MonitorArea(borrowed_ptr(m_state), this);
            this->controlArea = new ControlArea(borrowed_ptr(m_state), this);

            this->mainLayout = new QGridLayout;
            this->mainLayout->setContentsMargins(0, 0, 0, 0);
            this->mainLayout->setSpacing(0);
            // [XXX] keep it align unset, or the expansion does not work properly
            this->mainLayout->addWidget(this->monitorArea, 0, 0, -1, -1);
            this->mainLayout->addWidget(this->controlArea, 0, 0, -1, -1, Qt::AlignBottom);

            this->setLayout(this->mainLayout);
        }

        Window::~Window(){};

        void Window::changeEvent(QEvent* event) {
            if (event->type() == QEvent::WindowStateChange) {
                if (this->isFullScreen()) {
                    Q_EMIT this->controlArea->hide_control();
                } else {
                    Q_EMIT this->controlArea->show_control();
                }
            }
        }

        void Window::on_seek(const akashi::core::Rational& time) {
            if (can_seek(m_state)) {
                m_state->m_atomic_state.ui_can_seek.store(false);
                this->controlArea->set_slider_movable(false);
                this->monitorArea->seek(time);
                m_state->m_atomic_state.icon_play_state.store(akashi::state::PlayState::PAUSED);
                Q_EMIT this->state_changed(m_state->m_atomic_state.icon_play_state.load());
            }
        }

        void Window::on_frame_step(void) {
            if (can_seek(m_state)) {
                m_state->m_atomic_state.ui_can_seek.store(false);
                this->controlArea->set_slider_movable(false);
                this->monitorArea->frame_step();
                m_state->m_atomic_state.icon_play_state.store(akashi::state::PlayState::PAUSED);
                Q_EMIT this->state_changed(m_state->m_atomic_state.icon_play_state.load());
            }
        }

        void Window::on_frame_back_step(void) {
            if (can_seek(m_state)) {
                m_state->m_atomic_state.ui_can_seek.store(false);
                this->controlArea->set_slider_movable(false);
                this->monitorArea->frame_back_step();
                m_state->m_atomic_state.icon_play_state.store(akashi::state::PlayState::PAUSED);
                Q_EMIT this->state_changed(m_state->m_atomic_state.icon_play_state.load());
            }
        }

        void Window::on_forward_jump(const akashi::core::Rational& rel_pos) {
            if (can_seek(m_state)) {
                m_state->m_atomic_state.ui_can_seek.store(false);
                this->controlArea->set_slider_movable(false);
                this->monitorArea->relative_seek(rel_pos);
                m_state->m_atomic_state.icon_play_state.store(akashi::state::PlayState::PAUSED);
                Q_EMIT this->state_changed(m_state->m_atomic_state.icon_play_state.load());
            }
        }

        void Window::on_backward_jump(const akashi::core::Rational& rel_pos) {
            if (can_seek(m_state)) {
                m_state->m_atomic_state.ui_can_seek.store(false);
                this->controlArea->set_slider_movable(false);
                this->monitorArea->relative_seek(Rational(-1, 1) * rel_pos);
                m_state->m_atomic_state.icon_play_state.store(akashi::state::PlayState::PAUSED);
                Q_EMIT this->state_changed(m_state->m_atomic_state.icon_play_state.load());
            }
        }

        void Window::on_state_change(const akashi::state::PlayState& new_state) {
            switch (new_state) {
                case akashi::state::PlayState::STOPPED:
                case akashi::state::PlayState::PAUSED: {
                    this->monitorArea->pause();
                    break;
                }
                case akashi::state::PlayState::PLAYING: {
                    if (!m_state->get_evalbuf_dequeue_ready()) {
                        AKLOG_WARNN("Window::on_state_change(): kron not ready");
                        return;
                    }
                    this->monitorArea->play();
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
                    if (!m_state->get_evalbuf_dequeue_ready()) {
                        AKLOG_WARNN("Window::on_state_toggle(): kron not ready");
                        return;
                    }

                    m_state->m_atomic_state.icon_play_state.store(
                        akashi::state::PlayState::PLAYING);
                    this->monitorArea->play();
                    break;
                }
                case akashi::state::PlayState::PLAYING: {
                    m_state->m_atomic_state.icon_play_state.store(akashi::state::PlayState::PAUSED);
                    this->monitorArea->pause();
                    break;
                }
                default: {
                }
            }
            Q_EMIT this->state_changed(m_state->m_atomic_state.icon_play_state.load());
        }

        void Window::on_time_change(akashi::core::Fraction& time_frac) {
            Q_EMIT this->time_changed(to_rational(time_frac));
        }

        void Window::on_render_prof_updated(akashi::core::RenderProfile& render_prof) {
            Q_EMIT this->render_prof_changed(render_prof);
        }

        void Window::on_seek_completed(void) {
            m_state->m_atomic_state.ui_can_seek.store(true);
            this->controlArea->set_slider_movable(true);
        }

    }
}
