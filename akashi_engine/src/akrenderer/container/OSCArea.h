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

        class OSCWidget;

        class OSCArea final : public QWidget {
            Q_OBJECT
          public:
            explicit OSCArea(core::borrowed_ptr<akashi::state::AKState> state, QWidget* parent = 0);

            void resize_osc(int w, int h);

          protected:
            virtual void showEvent(QShowEvent* event) override;
            virtual void enterEvent(QEvent* event) override;
            virtual void leaveEvent(QEvent* event) override;

            virtual void mousePressEvent(QMouseEvent* event) override;
            virtual void mouseReleaseEvent(QMouseEvent* event) override;
            virtual void mouseMoveEvent(QMouseEvent* event) override;

          public Q_SLOTS:
            void show_control(void);
            void hide_control(void);
            void update_osc(void);

          private:
            QVBoxLayout* m_osc_area_layout;
            OSCWidget* m_osc_widget;
            QLabel* m_event_proxy;
            QGridLayout* m_main_layout;
            QPoint m_old_pos = {0, 0};
            bool m_can_move = false;
          Q_SIGNALS:
          public Q_SLOTS:
        };

    }
}
