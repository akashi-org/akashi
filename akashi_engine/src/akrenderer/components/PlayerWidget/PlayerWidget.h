#pragma once

#include <libakplayer/akplayer.h>
#include <libakevent/akevent.h>
#include <libakcore/element.h>
#include <libakcore/memory.h>
#include <libakcore/rational.h>

#include <QOpenGLWidget>

class QTimer;

namespace akashi {
    namespace state {
        class AKState;
        enum class PlayState;
    }

    namespace ui {

        class PlayerWidget final : public QOpenGLWidget {
            Q_OBJECT

            using RenderProfile = akashi::core::RenderProfile;

            // If the cursor remains stationary after the time below passed, the cursor gets hidden
            // automatically. This behavior is only applicable when m_enable_smart_cursor is true.
            static const int CURSOR_INTERVAL_MS = 1500;

          public:
            explicit PlayerWidget(akashi::core::borrowed_ptr<akashi::state::AKState> state,
                                  akashi::core::borrowed_ptr<akashi::player::AKPlayer> player,
                                  QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
            virtual ~PlayerWidget(void);
            void play(void);
            void pause(void);
            void seek(const akashi::core::Rational& pos);
            void relative_seek(const double ratio);
            void frame_seek(int nframes);
            void frame_step(void);
            void frame_back_step(void);
            core::Rational current_time(void);
            void inline_eval(const std::string& file_path, const std::string& elem_name);
            void set_volume(const double volume);

          Q_SIGNALS:
            void closed(void);
            void render_prof_updated(const akashi::core::RenderProfile& render_prof);
            void time_changed(const akashi::core::Rational& time);
            void play_state_changed(const akashi::state::PlayState& play_state);
            void seek_completed(void);

          protected:
            void initializeGL() override;
            void paintGL() override;

            virtual void mouseMoveEvent(QMouseEvent*) override;
            virtual void enterEvent(QEvent* event) override;
            virtual void leaveEvent(QEvent* event) override;

          private Q_SLOTS:

          private:
            static void on_update(void* ctx);
            static void on_event(void* evt_ctx, akashi::event::Event evt);
            void handle_update(void);
            void handle_player_event(akashi::event::Event evt);

          private:
            akashi::core::borrowed_ptr<akashi::state::AKState> m_state;
            akashi::core::borrowed_ptr<akashi::player::AKPlayer> m_player;
            QTimer* m_cursor_timer;
            bool m_enable_smart_cursor = true; // if true, hide the cursor automatically
        };

    }
}
