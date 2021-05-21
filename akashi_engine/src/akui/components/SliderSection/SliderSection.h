#pragma once

#include <libakcore/rational.h>
#include <libakcore/element.h>
#include <libakcore/memory.h>
#include <libakstate/akstate.h>

#include <QKeyEvent>
#include <QSlider>
#include <QStyleOptionSlider>
#include <QWidget>

class QHBoxLayout;

namespace akashi {
    namespace state {
        class AKState;
    }

    namespace ui {

        class AKVideoSlider final : public QSlider {
            Q_OBJECT

          public:
            explicit AKVideoSlider(Qt::Orientation, QWidget* parent = nullptr) {
                Q_UNUSED(parent);
            };
            explicit AKVideoSlider(QWidget* parent = nullptr) { Q_UNUSED(parent); };
            virtual ~AKVideoSlider() = default;

            bool movable(void) const { return m_movable; }

            void setMovable(bool movable) { m_movable = movable; }

          Q_SIGNALS:
            void rightKeyPressed(void);
            void ctrlRightKeyPressed(void);
            void shiftRightKeyPressed(void);
            void leftKeyPressed(void);
            void ctrlLeftKeyPressed(void);
            void shiftLeftKeyPressed(void);

          protected:
            void mouseMoveEvent(QMouseEvent* event) override;
            void mousePressEvent(QMouseEvent* event) override;
            void wheelEvent(QWheelEvent*) override;
            void keyPressEvent(QKeyEvent* event) override;

          private:
            bool m_movable = true;
        };

        class SliderSection final : public QWidget {
            Q_OBJECT
          public:
            explicit SliderSection(core::borrowed_ptr<akashi::state::AKState> state,
                                   QWidget* parent = 0);
            void set_slider_movable(bool movable);

          private:
            AKVideoSlider* slider;
            QHBoxLayout* layout;
            const int short_jump_value = 1; // sec
            const int long_jump_value = 10; // sec
            akashi::core::Rational m_unit = akashi::core::Rational(1, 30);
          Q_SIGNALS:
            void slider_moved(const akashi::core::Rational&);
            void slider_pressed(const akashi::state::PlayState&);
            void frame_step(void);
            void frame_back_step(void);
            void forward_jump(const akashi::core::Rational&);
            void backward_jump(const akashi::core::Rational&);
          public Q_SLOTS:
            void on_render_prof_changed(const akashi::core::RenderProfile& render_prof);
            void set_slider_value(const akashi::core::Rational& pos);
        };

    }
}
