#pragma once

#include <libakevent/akevent.h>
#include <libakcore/element.h>
#include <libakcore/memory.h>
#include <libakcore/rational.h>
#include <libakgraphics/item.h>
#include <libakgraphics/osc.h>

#include <QOpenGLWidget>

class QTimer;

namespace akashi {
    namespace state {
        class AKState;
        enum class PlayState;
    }
    namespace graphics {
        class OSCWidget;
    }

    namespace ui {

        class OSCWidget final : public QOpenGLWidget {
            Q_OBJECT

            // If the cursor remains stationary after the time below passed, the cursor gets hidden
            // automatically. This behavior is only applicable when m_enable_smart_cursor is true.
            static const int CURSOR_INTERVAL_MS = 1500;

            static const int MOUSE_HOLD_INTERVAL_MS = 50;

          public:
            explicit OSCWidget(akashi::core::borrowed_ptr<akashi::state::AKState> state,
                               QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
            virtual ~OSCWidget(void);

          Q_SIGNALS:
            void closed(void);
            void playBtn_clicked(void);
            void volume_changed(double gain);
            void frame_seek(int nframes);
            void seekbar_pressed(const akashi::core::Rational&);
            void seekbar_moved(const akashi::core::Rational&);
            void seekbar_released(const akashi::core::Rational&);

          public Q_SLOTS:
            void update_current_time(const akashi::core::Rational& current_time);
            void update_duration(const akashi::core::RenderProfile& render_prof);

          protected:
            void initializeGL() override;
            void paintGL() override;
            void resizeGL(int w, int h) override;

            virtual void mousePressEvent(QMouseEvent*) override;
            virtual void mouseMoveEvent(QMouseEvent*) override;
            virtual void mouseReleaseEvent(QMouseEvent*) override;
            virtual void enterEvent(QEvent* event) override;
            virtual void leaveEvent(QEvent* event) override;

          private Q_SLOTS:

          private:
            akashi::graphics::RenderParams current_render_params();
            static void on_event(void* evt_ctx, akashi::graphics::OSCInnerEvent evt);
            void handle_osc_event(akashi::graphics::OSCInnerEvent evt);

          private:
            akashi::core::owned_ptr<akashi::graphics::OSCWidget> m_osc;
            QTimer* m_cursor_timer;
            bool m_enable_smart_cursor = true; // if true, hide the cursor automatically
            QTimer* m_mouse_hold_timer;
            Qt::MouseButton m_last_pressed_btn = Qt::NoButton;
        };

    }
}
