#include "./OSCArea.h"

#include "../components/OSCWidget/OSCWidget.h"
#include "../window.h"

#include <libakcore/rational.h>
#include <libakcore/element.h>

#include <QVBoxLayout>
#include <QLabel>
#include <QMouseEvent>

namespace akashi {
    namespace ui {

        OSCArea::OSCArea(core::borrowed_ptr<akashi::state::AKState> state, QWidget* parent)
            : QWidget(parent) {
            this->setObjectName("osc_area");
            this->m_osc_widget = new OSCWidget(state);

            // m_osc_widget -> parent
            QObject::connect(this->m_osc_widget, SIGNAL(playBtn_clicked(void)), this->window(),
                             SLOT(on_state_toggle(void)));
            QObject::connect(this->m_osc_widget, SIGNAL(volume_changed(double)), this->window(),
                             SLOT(on_volume_changed(double)));

            QObject::connect(this->m_osc_widget, &OSCWidget::frame_seek,
                             static_cast<Window*>(this->window()), &Window::on_frame_seek);

            QObject::connect(this->m_osc_widget, &OSCWidget::seekbar_pressed,
                             static_cast<Window*>(this->window()), &Window::on_seekbar_pressed);
            QObject::connect(this->m_osc_widget, &OSCWidget::seekbar_moved,
                             static_cast<Window*>(this->window()), &Window::on_seekbar_moved);
            QObject::connect(this->m_osc_widget, &OSCWidget::seekbar_released,
                             static_cast<Window*>(this->window()), &Window::on_seekbar_released);

            // parent -> m_osc_widget
            QObject::connect(
                static_cast<Window*>(this->window()), &Window::state_changed,
                [this](const akashi::state::PlayState&) { this->m_osc_widget->update(); });

            QObject::connect(static_cast<Window*>(this->window()), &Window::volume_changed,
                             [this](double) { this->m_osc_widget->update(); });

            QObject::connect(static_cast<Window*>(this->window()), &Window::time_changed,
                             [this](const akashi::core::Rational& current_time) {
                                 this->m_osc_widget->update_current_time(current_time);
                             });

            QObject::connect(static_cast<Window*>(this->window()), &Window::render_prof_changed,
                             [this](const akashi::core::RenderProfile& render_prof) {
                                 this->m_osc_widget->update_duration(render_prof);
                             });

            this->m_osc_area_layout = new QVBoxLayout;
            this->m_osc_area_layout->setContentsMargins(0, 0, 0, 0);
            this->m_osc_area_layout->setSpacing(0);
            // this->controlAreaLayout->addWidget(this->slider);
            this->m_osc_area_layout->addWidget(this->m_osc_widget);

            this->m_event_proxy = new QLabel;
            this->m_event_proxy->setObjectName("event_proxy");

            this->m_main_layout = new QGridLayout;
            this->m_main_layout->setContentsMargins(0, 0, 0, 0);
            this->m_main_layout->setSpacing(0);
            // [XXX] keep it align unset, or the expansion does not work properly
            this->m_main_layout->addWidget(this->m_event_proxy, 0, 0, -1, -1);
            this->m_main_layout->addLayout(this->m_osc_area_layout, 0, 0, -1, -1);

            this->setLayout(this->m_main_layout);
        }

        void OSCArea::showEvent(QShowEvent*) {
            // auto ch = this->m_osc_area_layout->geometry().height();
            // m_event_proxy->setStyleSheet(
            //     QString::fromStdString("color: transparent; padding-top:" + std::to_string(ch /
            //     4) +
            //                            "px; padding-bottom: " + std::to_string(ch / 4) + "px"));
            m_event_proxy->setStyleSheet("color: transparent");
        }

        void OSCArea::enterEvent(QEvent*) { Q_EMIT this->show_control(); }

        void OSCArea::leaveEvent(QEvent*) {
            if (this->parentWidget()->isFullScreen()) {
                Q_EMIT this->hide_control();
            }
        }

        void OSCArea::mousePressEvent(QMouseEvent* event) {
            m_old_pos = event->globalPos();
            m_can_move = true;
        }

        void OSCArea::mouseReleaseEvent(QMouseEvent*) { m_can_move = false; }

        void OSCArea::mouseMoveEvent(QMouseEvent* event) {
            if (m_can_move && (event->buttons() & Qt::LeftButton)) {
                this->move(this->mapToParent(event->globalPos() - m_old_pos));
                m_old_pos = event->globalPos();
            }
        }

        void OSCArea::show_control(void) {
            // if (this->m_osc_widget->isHidden()) {
            //     this->m_osc_widget->show();
            // }
        }

        void OSCArea::hide_control(void) {
            // if (!this->m_osc_widget->isHidden()) {
            //     this->m_osc_widget->hide();
            // }
        }

    }
}
