#pragma once

#include <QLabel>
#include <QWidget>

class QHBoxLayout;

namespace akashi {
    namespace core {
        class Rational;
        struct RenderProfile;
    }
    namespace state {
        enum class PlayState;
    }

    namespace ui {

        class ClickableLabel final : public QLabel {
            Q_OBJECT

          public:
            explicit ClickableLabel(QWidget* parent = Q_NULLPTR,
                                    Qt::WindowFlags f = Qt::WindowFlags()) {
                Q_UNUSED(parent);
                Q_UNUSED(f);
            };
            virtual ~ClickableLabel() = default;

          Q_SIGNALS:
            void clicked(bool);

          protected:
            void mouseReleaseEvent(QMouseEvent* event) override {
                Q_UNUSED(event);
                Q_EMIT clicked(false);
            }
        };

        class WidgetSection final : public QWidget {
            Q_OBJECT
          public:
            explicit WidgetSection(QWidget* parent = 0);

          private:
            QHBoxLayout* layout;
            ClickableLabel* playBtn;
            QLabel* timecode;
            QLabel* sepLabel;
            QLabel* durationLabel;
            QLabel* volumeBtn;
            QLabel* fullscreenBtn;
            QLabel* menuBtn;
          Q_SIGNALS:
            void playBtn_clicked(void);
          public Q_SLOTS:
            void set_playBtn_icon(const akashi::state::PlayState& state);
            void set_timecode_pos(const akashi::core::Rational& pos);
            void set_durationLabel(const akashi::core::RenderProfile& render_prof);
        };
    }

}
