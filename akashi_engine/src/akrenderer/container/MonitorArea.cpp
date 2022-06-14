#include "./MonitorArea.h"

#include "../components/PlayerWidget/PlayerWidget.h"

#include <libakcore/rational.h>
#include <libakcore/element.h>

#include <QHBoxLayout>
#include <QPushButton>
#include <QWidget>

namespace akashi {
    namespace ui {

        MonitorArea::MonitorArea(akashi::core::borrowed_ptr<akashi::state::AKState> state,
                                 QWidget* parent)
            : QWidget(parent) {
            this->setObjectName("monitor_area");
            m_player = new PlayerWidget(state, this);

            // player -> parent
            QObject::connect(m_player, SIGNAL(time_changed(const akashi::core::Rational&)),
                             this->window(), SLOT(on_time_change(const akashi::core::Rational&)));
            QObject::connect(
                m_player, SIGNAL(render_prof_updated(const akashi::core::RenderProfile&)),
                this->window(), SLOT(on_render_prof_updated(const akashi::core::RenderProfile&)));
            QObject::connect(m_player, SIGNAL(play_state_changed(const akashi::state::PlayState&)),
                             this->window(),
                             SLOT(on_state_change(const akashi::state::PlayState&)));
            QObject::connect(m_player, SIGNAL(seek_completed(void)), this->window(),
                             SLOT(on_seek_completed(void)));

            m_monitorAreaLayout = new QHBoxLayout;
            m_monitorAreaLayout->setContentsMargins(0, 0, 0, 0);
            m_monitorAreaLayout->setSpacing(0);

            m_monitorAreaLayout->addWidget(m_player);

            this->setLayout(m_monitorAreaLayout);
        }

        void MonitorArea::play(void) { m_player->play(); }

        void MonitorArea::pause(void) { m_player->pause(); }

        void MonitorArea::seek(const akashi::core::Rational& pos) { m_player->seek(pos); }

        void MonitorArea::relative_seek(const double ratio) { m_player->relative_seek(ratio); }

        void MonitorArea::frame_seek(int nframes) { m_player->frame_seek(nframes); }

        void MonitorArea::frame_step(void) { m_player->frame_step(); }

        void MonitorArea::frame_back_step(void) { m_player->frame_back_step(); }

    }
}
