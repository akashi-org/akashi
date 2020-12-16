#pragma once

#include <libakcore/memory.h>

#include <QWidget>

class QGridLayout;

namespace akashi {
    namespace core {
        class Rational;
        struct Fraction;
        struct RenderProfile;
    }
    namespace state {
        class AKState;
        enum class PlayState;
    }

    namespace ui {
        class ControlArea;
        class MonitorArea;

        class Window final : public QWidget {
            Q_OBJECT
          public:
            explicit Window(akashi::core::borrowed_ptr<akashi::state::AKState> state,
                            QWidget* parent = 0);
            virtual ~Window();

          protected:
            virtual void changeEvent(QEvent* event) override;

          private:
            MonitorArea* monitorArea;
            ControlArea* controlArea;
            QGridLayout* mainLayout;
            akashi::core::borrowed_ptr<akashi::state::AKState> m_state;
          Q_SIGNALS:
            void state_changed(const akashi::state::PlayState& playState);
            void time_changed(const akashi::core::Rational& time);
            void render_prof_changed(const akashi::core::RenderProfile& render_prof);
          public Q_SLOTS:
            void on_seek(const akashi::core::Rational&);
            void on_frame_step(void);
            void on_frame_back_step(void);
            void on_forward_jump(const akashi::core::Rational&);
            void on_backward_jump(const akashi::core::Rational&);
            void on_state_change(const akashi::state::PlayState&);
            void on_state_toggle(void);
            void on_time_change(akashi::core::Fraction&);
            void on_render_prof_updated(akashi::core::RenderProfile&);
            void on_seek_completed(void);
        };

    }

}
