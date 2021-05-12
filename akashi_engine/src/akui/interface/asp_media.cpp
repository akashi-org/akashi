#include "./asp.h"
#include "../utils/widget.h"
#include "../window.h"
#include "../components/PlayerWidget/PlayerWidget.h"

#include <libakserver/akserver.h>
#include <libakcore/rational.h>

#include <QWidget>
#include <QImage>
#include <QBuffer>
#include <QByteArray>

using namespace akashi::core;

namespace akashi {
    namespace ui {

        ASPMediaAPIImpl::ASPMediaAPIImpl(QWidget* root) : m_root(root) {
            m_player = m_root->findChild<PlayerWidget*>("player_widget");
        }

        std::string ASPMediaAPIImpl::take_snapshot(void) {
            QImage image;

            QMetaObject::invokeMethod(
                m_player, [&]() { image = m_player->grabFramebuffer(); },
                Qt::BlockingQueuedConnection);

            QByteArray arr;
            QBuffer buffer(&arr);
            buffer.open(QIODevice::WriteOnly);
            image.save(&buffer, "PNG");

            QString b64str = buffer.buffer().toBase64();
            return b64str.toUtf8().constData();
        };

        bool ASPMediaAPIImpl::toggle_fullscreen(void) {
            return QMetaObject::invokeMethod(
                m_root, [&]() { static_cast<Window*>(m_root)->toggleFullScreen(); },
                Qt::BlockingQueuedConnection);
        }

        bool ASPMediaAPIImpl::seek(const int num, const int den) {
            return QMetaObject::invokeMethod(
                m_player, [&]() { m_player->seek(Rational(num, den)); },
                Qt::BlockingQueuedConnection);
        }

        bool ASPMediaAPIImpl::relative_seek(const int num, const int den) {
            return QMetaObject::invokeMethod(
                m_player, [&]() { m_player->relative_seek(Rational(num, den)); },
                Qt::BlockingQueuedConnection);
        }

        bool ASPMediaAPIImpl::frame_step(void) {
            return QMetaObject::invokeMethod(
                m_player, [&]() { m_player->frame_step(); }, Qt::BlockingQueuedConnection);
        }

        bool ASPMediaAPIImpl::frame_back_step(void) {
            return QMetaObject::invokeMethod(
                m_player, [&]() { m_player->frame_back_step(); }, Qt::BlockingQueuedConnection);
        }

        std::vector<int64_t> ASPMediaAPIImpl::current_time(void) {
            core::Rational current_time;
            QMetaObject::invokeMethod(
                m_player, [&]() { current_time = m_player->current_time(); },
                Qt::BlockingQueuedConnection);
            return {current_time.num(), current_time.den()};
        }

    }
}
