#include "./WidgetSection.h"

#include <libakcore/logger.h>
#include <libakcore/rational.h>
#include <libakcore/element.h>
#include <libakstate/akstate.h>

#include <QDebug>
#include <QFont>
#include <QFontDatabase>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QString>
#include <QTime>
#include <QWidget>
#include <cmath>

using namespace akashi::core;

namespace akashi {
    namespace ui {
        WidgetSection::WidgetSection(QWidget* parent) : QWidget(parent) {
            this->setObjectName("widget_section");

            QFont font;
            font.setFamily("FontAwesome");

            this->setStyleSheet("color: white;");

            this->playBtn = new ClickableLabel;
            this->playBtn->setObjectName("play_btn");
            // [TODO] maybe we should not use an absolute number
            this->playBtn->setStyleSheet("margin-left: 10px;");
            this->playBtn->setFont(font);
            this->playBtn->setText("\uf04b"); // [TODO] should be taken from state

            // [TODO] cannot set fixed pixel size for the text in QLabel
            // So far, we have not found a way to set a fixed pixel size for the text, and the
            // pixel size is changed slightly by each time we set the new value for the label.
            // As a workaround for this issue, we set the text alignment to left so that it will
            // mitigate the unstable pixel size problems.
            this->timecode = new QLabel;
            this->timecode->setObjectName("timecode");
            this->timecode->setFixedWidth(this->size().width() * 0.17);
            // [TODO] maybe we should not use an absolute number
            this->timecode->setStyleSheet("padding-left: 5px; qproperty-alignment: AlignLeft;");
            this->timecode->setText("00:00:00.000"); // [TODO] should be taken from state

            this->sepLabel = new QLabel;
            this->sepLabel->setObjectName("sepLabel");
            this->sepLabel->setStyleSheet("qproperty-alignment: AlignLeft;");
            // this->sepLabel->setFixedWidth(this->size().width() * 0.18);
            this->sepLabel->setText("/");

            this->durationLabel = new QLabel;
            this->durationLabel->setObjectName("duration_label");
            this->durationLabel->setStyleSheet(
                "padding-left: 5px; qproperty-alignment: AlignLeft;");
            // [TODO] maybe we should not use an absolute number
            this->durationLabel->setText("00:00:00.000"); // [TODO] should be taken from state

            this->volumeBtn = new QLabel;
            this->volumeBtn->setObjectName("volume_btn");
            this->volumeBtn->setStyleSheet("qproperty-alignment: AlignCenter;");
            this->volumeBtn->setFont(font);
            this->volumeBtn->setText("\uf028");
            // [TODO] delete the line below after the implementation is finished
            this->volumeBtn->setVisible(false);

            this->fullscreenBtn = new QLabel;
            this->fullscreenBtn->setObjectName("fullscreen_btn");
            this->fullscreenBtn->setStyleSheet("qproperty-alignment: AlignCenter;");
            this->fullscreenBtn->setFont(font);
            this->fullscreenBtn->setText("\uf065");
            // [TODO] delete the line below after the implementation is finished
            this->fullscreenBtn->setVisible(false);

            this->menuBtn = new QLabel;
            this->menuBtn->setObjectName("menu_btn");
            this->menuBtn->setStyleSheet("qproperty-alignment: AlignCenter;");
            this->menuBtn->setFont(font);
            this->menuBtn->setText("\uf142");
            // [TODO] delete the line below after the implementation is finished
            this->menuBtn->setVisible(false);

            this->layout = new QHBoxLayout;
            this->layout->setContentsMargins(0, 0, 0, this->size().height() * 0.01);
            this->layout->setSpacing(0);

            this->layout->addWidget(this->playBtn, 1);
            this->layout->addWidget(this->timecode, 2);
            this->layout->addWidget(this->sepLabel, 0);
            this->layout->addWidget(this->durationLabel, 2);
            this->layout->addStretch(20);
            this->layout->addWidget(this->volumeBtn, 1);
            this->layout->addWidget(this->fullscreenBtn, 1);
            this->layout->addWidget(this->menuBtn, 1);

            this->setLayout(this->layout);

            QObject::connect(this->playBtn, &ClickableLabel::clicked, this,
                             &WidgetSection::playBtn_clicked);
        }

        void WidgetSection::set_playBtn_icon(const akashi::state::PlayState& state) {
            switch (state) {
                case akashi::state::PlayState::STOPPED:
                case akashi::state::PlayState::PAUSED: {
                    this->playBtn->setText("\uf04b");
                    break;
                }
                case akashi::state::PlayState::PLAYING: {
                    this->playBtn->setText("\uf04c");
                    break;
                }
                default: {
                }
            }
        }

        void WidgetSection::set_timecode_pos(const akashi::core::Rational& pos) {
            double sec = 0;
            double ms = 0;
            QTime qtime(0, 0, 0, 0);
            ms = std::modf(pos.to_decimal(), &sec);
            qtime = qtime.addSecs(sec).addMSecs(ms * 1000);
            this->timecode->setText(qtime.toString("hh:mm:ss.zzz"));
        }

        void WidgetSection::set_durationLabel(const akashi::core::RenderProfile& render_prof) {
            double sec = 0;
            double ms = 0;
            QTime qtime(0, 0, 0, 0);
            ms = std::modf(to_rational(render_prof.duration).to_decimal(), &sec);
            qtime = qtime.addSecs(sec).addMSecs(ms * 1000);
            this->durationLabel->setText(qtime.toString("hh:mm:ss.zzz"));
        }

    }
}
