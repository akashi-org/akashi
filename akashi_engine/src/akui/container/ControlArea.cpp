#include "./ControlArea.h"

#include "../components/SliderSection/SliderSection.h"
#include "../components/WidgetSection/WidgetSection.h"

#include <libakcore/rational.h>
#include <libakcore/element.h>

#include <QVBoxLayout>

namespace akashi {
    namespace ui {

        ControlArea::ControlArea(core::borrowed_ptr<akashi::state::AKState> state, QWidget* parent)
            : QWidget(parent) {
            this->setObjectName("control_area");
            this->m_slider_section = new SliderSection(state);
            this->m_widget_section = new WidgetSection;

            // m_widget_section, m_slider_section -> parent
            QObject::connect(this->m_widget_section, SIGNAL(playBtn_clicked(void)), this->window(),
                             SLOT(on_state_toggle(void)));

            QObject::connect(this->m_slider_section,
                             SIGNAL(slider_moved(const akashi::core::Rational&)), this->window(),
                             SLOT(on_seek(const akashi::core::Rational&)));
            QObject::connect(
                this->m_slider_section, SIGNAL(slider_pressed(const akashi::state::PlayState&)),
                this->window(), SLOT(on_state_update(const akashi::state::PlayState&)));
            QObject::connect(this->m_slider_section, SIGNAL(frame_step(void)), this->window(),
                             SLOT(on_frame_step(void)));
            QObject::connect(this->m_slider_section, SIGNAL(frame_back_step(void)), this->window(),
                             SLOT(on_frame_back_step(void)));
            QObject::connect(this->m_slider_section,
                             SIGNAL(forward_jump(const akashi::core::Rational&)), this->window(),
                             SLOT(on_forward_jump(const akashi::core::Rational&)));
            QObject::connect(this->m_slider_section,
                             SIGNAL(backward_jump(const akashi::core::Rational&)), this->window(),
                             SLOT(on_backward_jump(const akashi::core::Rational&)));

            // parent -> m_widget_section
            QObject::connect(this->window(), SIGNAL(state_changed(const akashi::state::PlayState&)),
                             this->m_widget_section,
                             SLOT(set_playBtn_icon(const akashi::state::PlayState&)));
            QObject::connect(this->window(), SIGNAL(time_changed(const akashi::core::Rational&)),
                             this->m_widget_section,
                             SLOT(set_timecode_pos(const akashi::core::Rational&)));
            QObject::connect(this->window(),
                             SIGNAL(render_prof_changed(const akashi::core::RenderProfile&)),
                             this->m_widget_section,
                             SLOT(set_durationLabel(const akashi::core::RenderProfile&)));

            // parent -> m_slider_section
            QObject::connect(this->window(), SIGNAL(time_changed(const akashi::core::Rational&)),
                             this->m_slider_section,
                             SLOT(set_slider_value(const akashi::core::Rational&)));
            QObject::connect(this->window(),
                             SIGNAL(render_prof_changed(const akashi::core::RenderProfile&)),
                             this->m_slider_section,
                             SLOT(on_render_prof_changed(const akashi::core::RenderProfile&)));

            this->m_control_area_layout = new QVBoxLayout;
            this->m_control_area_layout->setContentsMargins(0, 0, 0, 0);
            this->m_control_area_layout->setSpacing(0);
            // this->controlAreaLayout->addWidget(this->slider);
            this->m_control_area_layout->addWidget(this->m_widget_section);
            this->m_control_area_layout->addWidget(this->m_slider_section);

            this->m_event_proxy = new QLabel;
            this->m_event_proxy->setObjectName("event_proxy");

            this->m_main_layout = new QGridLayout;
            this->m_main_layout->setContentsMargins(0, 0, 0, 0);
            this->m_main_layout->setSpacing(0);
            // [XXX] keep it align unset, or the expansion does not work properly
            this->m_main_layout->addWidget(this->m_event_proxy, 0, 0, -1, -1);
            this->m_main_layout->addLayout(this->m_control_area_layout, 0, 0, -1, -1);

            this->setLayout(this->m_main_layout);
        }

        void ControlArea::set_slider_movable(bool movable) {
            this->m_slider_section->set_slider_movable(movable);
        }

        void ControlArea::showEvent(QShowEvent*) {
            auto ch = this->m_control_area_layout->geometry().height();
            m_event_proxy->setStyleSheet(
                QString::fromStdString("color: transparent; padding-top:" + std::to_string(ch / 4) +
                                       "px; padding-bottom: " + std::to_string(ch / 4) + "px"));
        }

        void ControlArea::enterEvent(QEvent*) { Q_EMIT this->show_control(); }

        void ControlArea::leaveEvent(QEvent*) {
            if (this->parentWidget()->isFullScreen()) {
                Q_EMIT this->hide_control();
            }
        }

        void ControlArea::show_control(void) {
            if (this->m_widget_section->isHidden()) {
                this->m_widget_section->show();
                this->m_slider_section->show();
            }
        }

        void ControlArea::hide_control(void) {
            if (!this->m_widget_section->isHidden()) {
                this->m_widget_section->hide();
                this->m_slider_section->hide();
            }
        }

    }
}
