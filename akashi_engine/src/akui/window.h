#pragma once

#include <libakcore/memory.h>

#include <QFrame>
#include <QLabel>

class QGridLayout;
class QEvent;
class QMouseEvent;
class QResizeEvent;
class QSizeGrip;

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

        class ExitButton final : public QLabel {
            Q_OBJECT

          public:
            explicit ExitButton(QWidget* parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags());
            virtual ~ExitButton();

          protected:
            virtual void mouseReleaseEvent(QMouseEvent* event) override;
            virtual void enterEvent(QEvent* event) override;
            virtual void leaveEvent(QEvent* event) override;

          private:
            std::string m_base_css;
        };

        class Window final : public QFrame {
            Q_OBJECT
          public:
            explicit Window(akashi::core::borrowed_ptr<akashi::state::AKState> state,
                            QWidget* parent = 0);
            virtual ~Window();
            void toggleFullScreen();
            void setTransparent(bool transparent);
            void changePlayState(const state::PlayState& new_state);

          protected:
            virtual void showEvent(QShowEvent* event) override;
            virtual void changeEvent(QEvent* event) override;
            virtual void mousePressEvent(QMouseEvent* event) override;
            virtual void mouseReleaseEvent(QMouseEvent* event) override;
            virtual void mouseMoveEvent(QMouseEvent* event) override;
            virtual void mouseDoubleClickEvent(QMouseEvent* event) override;

          private:
            MonitorArea* m_monitorArea;
            ControlArea* m_controlArea;
            QGridLayout* m_mainLayout;
            ExitButton* m_exitBtn;
            QSizeGrip* m_sizeGrip;
            QPoint m_lastMouse;
            QSize m_origSize;
            akashi::core::borrowed_ptr<akashi::state::AKState> m_state;

          Q_SIGNALS:
            void state_changed(const akashi::state::PlayState& playState);
            void time_changed(const akashi::core::Rational& time);
            void render_prof_changed(const akashi::core::RenderProfile& render_prof);
            void window_activated(void);
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
