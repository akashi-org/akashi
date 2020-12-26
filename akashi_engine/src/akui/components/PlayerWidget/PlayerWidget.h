#pragma once

#include <libakplayer/akplayer.h>
#include <libakevent/akevent.h>
#include <libakcore/element.h>
#include <libakcore/memory.h>
#include <libakcore/rational.h>

#include <QOpenGLWidget>

namespace akashi {
    namespace state {
        class AKState;
        enum class PlayState;
    }

    namespace ui {

        class PlayerWidget final : public QOpenGLWidget {
            Q_OBJECT

            using RenderProfile = akashi::core::RenderProfile;
            using Fraction = akashi::core::Fraction;

          public:
            explicit PlayerWidget(akashi::core::borrowed_ptr<akashi::state::AKState> state,
                                  QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
            virtual ~PlayerWidget(void);
            void play(void);
            void pause(void);
            void seek(const akashi::core::Rational& pos);
            void relative_seek(const akashi::core::Rational& rel_pos);
            void frame_step(void);
            void frame_back_step(void);

          Q_SIGNALS:
            void closed(void);
            void render_prof_updated(akashi::core::RenderProfile& render_prof);
            void time_changed(akashi::core::Fraction& time);
            void play_state_changed(const akashi::state::PlayState& play_state);
            void seek_completed(void);

          protected:
            void initializeGL() override;
            void paintGL() override;
          private Q_SLOTS:

          private:
            static void on_update(void* ctx);
            static void on_event(void* evt_ctx, akashi::event::Event evt);
            void handle_update(void);
            void handle_player_event(akashi::event::Event evt);

          private:
            akashi::core::borrowed_ptr<akashi::state::AKState> m_state;
            akashi::core::owned_ptr<akashi::player::AKPlayer> m_player;
        };

    }
}
