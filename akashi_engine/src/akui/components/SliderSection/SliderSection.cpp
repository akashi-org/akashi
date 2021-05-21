#include "./SliderSection.h"

#include <libakcore/rational.h>
#include <libakcore/logger.h>
#include <libakstate/akstate.h>

#include <QHBoxLayout>
#include <QSlider>
#include <QWidget>

using namespace akashi::core;

namespace akashi {
    namespace ui {

        void AKVideoSlider::mouseMoveEvent(QMouseEvent* event) {
            if (m_movable) {
                QSlider::mouseMoveEvent(event);
            }
        }

        void AKVideoSlider::mousePressEvent(QMouseEvent* event) {
            // By default, QSlider will move its step when it is clicked
            // This behavior is unwanted, so workaround below is needed
            auto before_page_step = this->pageStep();
            auto before_single_step = this->singleStep();

            this->setPageStep(0);
            this->setSingleStep(0);
            QSlider::mousePressEvent(event);
            this->setPageStep(before_page_step);
            this->setSingleStep(before_single_step);
        }

        void AKVideoSlider::wheelEvent(QWheelEvent*) {
            // disable wheel event
        }

        void AKVideoSlider::keyPressEvent(QKeyEvent* event) {
            switch (event->key()) {
                case Qt::Key_Right: {
                    switch (event->modifiers()) {
                        case Qt::ControlModifier: {
                            Q_EMIT this->ctrlRightKeyPressed();
                            break;
                        }
                        case Qt::ShiftModifier: {
                            Q_EMIT this->shiftRightKeyPressed();
                            break;
                        }
                        default: {
                            Q_EMIT this->rightKeyPressed();
                        }
                    }
                    break;
                }
                case Qt::Key_Left: {
                    switch (event->modifiers()) {
                        case Qt::ControlModifier: {
                            Q_EMIT this->ctrlLeftKeyPressed();
                            break;
                        }
                        case Qt::ShiftModifier: {
                            Q_EMIT this->shiftLeftKeyPressed();
                            break;
                        }
                        default: {
                            Q_EMIT this->leftKeyPressed();
                        }
                    }
                    break;
                }
                default: {
                }
            }
        }

        SliderSection::SliderSection(core::borrowed_ptr<akashi::state::AKState> state,
                                     QWidget* parent)
            : QWidget(parent) {
            {
                std::lock_guard<std::mutex> lock(state->m_prop_mtx);
                m_unit = Rational(1, 1) / state->m_prop.fps;
            }

            this->setObjectName("slider_section");

            this->slider = new AKVideoSlider;
            this->slider->setObjectName("slider");
            this->slider->setOrientation(Qt::Horizontal);
            this->slider->setRange(0, 0);
            this->slider->setValue(0);
            this->slider->setSingleStep(1);

            this->layout = new QHBoxLayout;
            const double v_margin = 0;
            const double h_margin = 0.008;
            this->layout->setContentsMargins(this->size().width() * h_margin, v_margin,
                                             this->size().width() * h_margin, v_margin);
            this->layout->setSpacing(0);
            this->layout->addWidget(this->slider);

            this->setLayout(this->layout);

            QObject::connect(this->slider, &AKVideoSlider::sliderMoved, [=](int pos) {
                Q_EMIT this->slider_moved(Rational(pos, 1) * m_unit);
            });

            QObject::connect(this->slider, &AKVideoSlider::sliderPressed, [=]() {
                Q_EMIT this->slider_pressed(akashi::state::PlayState::PAUSED);
            });

            QObject::connect(this->slider, &AKVideoSlider::ctrlRightKeyPressed,
                             [=]() { Q_EMIT this->frame_step(); });

            QObject::connect(this->slider, &AKVideoSlider::ctrlLeftKeyPressed,
                             [=]() { Q_EMIT this->frame_back_step(); });

            QObject::connect(this->slider, &AKVideoSlider::rightKeyPressed,
                             [=]() { Q_EMIT this->forward_jump(this->short_jump_value); });

            QObject::connect(this->slider, &AKVideoSlider::leftKeyPressed,
                             [=]() { Q_EMIT this->backward_jump(this->short_jump_value); });

            QObject::connect(this->slider, &AKVideoSlider::shiftRightKeyPressed,
                             [=]() { Q_EMIT this->forward_jump(this->long_jump_value); });

            QObject::connect(this->slider, &AKVideoSlider::shiftLeftKeyPressed,
                             [=]() { Q_EMIT this->backward_jump(this->long_jump_value); });
        }

        void SliderSection::set_slider_movable(bool movable) { this->slider->setMovable(movable); }

        void SliderSection::on_render_prof_changed(const akashi::core::RenderProfile& render_prof) {
            // m_unit = Rational(1, render_prof.fps);
            blockSignals(true);
            this->slider->setRange(
                0, static_cast<int>((to_rational(render_prof.duration) / m_unit).to_decimal()));
            blockSignals(false);
        }

        void SliderSection::set_slider_value(const akashi::core::Rational& pos) {
            // [XXX] prevent slider from emitting sliderMoved signals
            blockSignals(true);
            this->slider->setValue(static_cast<int>((pos / m_unit).to_decimal()));
            blockSignals(false);
        }

    }
}
