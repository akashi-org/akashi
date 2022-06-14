#pragma once

#include <libakcore/rational.h>
#include <libakcore/memory.h>

#include <QWidget>

class QHBoxLayout;

namespace akashi {
    namespace state {
        class AKState;
    }

    namespace ui {

        class PlayerWidget;
        class MonitorArea final : public QWidget {
            Q_OBJECT
          public:
            explicit MonitorArea(akashi::core::borrowed_ptr<akashi::state::AKState> state,
                                 QWidget* parent = 0);
            void play(void);
            void pause(void);
            void seek(const akashi::core::Rational& pos);
            void relative_seek(const double ratio);
            void frame_seek(int nframes);
            void frame_step(void);
            void frame_back_step(void);

          private:
            PlayerWidget* m_player;
            QHBoxLayout* m_monitorAreaLayout;
          Q_SIGNALS:
          public Q_SLOTS:
        };

    }
}
