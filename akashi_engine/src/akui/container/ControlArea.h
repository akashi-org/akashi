#pragma once

#include <libakcore/memory.h>

#include <QWidget>

class QVBoxLayout;
class QGridLayout;
class QLabel;

namespace akashi {
    namespace state {
        class AKState;
    }
    namespace ui {

        class SliderSection;
        class WidgetSection;

        class ControlArea final : public QWidget {
            Q_OBJECT
          public:
            explicit ControlArea(core::borrowed_ptr<akashi::state::AKState> state,
                                 QWidget* parent = 0);

            void set_slider_movable(bool movable);

          protected:
            virtual void showEvent(QShowEvent* event) override;
            virtual void enterEvent(QEvent* event) override;
            virtual void leaveEvent(QEvent* event) override;

          public Q_SLOTS:
            void show_control(void);
            void hide_control(void);

          private:
            QVBoxLayout* m_control_area_layout;
            SliderSection* m_slider_section;
            WidgetSection* m_widget_section;
            QLabel* m_event_proxy;
            QGridLayout* m_main_layout;
          Q_SIGNALS:
          public Q_SLOTS:
        };

    }
}
