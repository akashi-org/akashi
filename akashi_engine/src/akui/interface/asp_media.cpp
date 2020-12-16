#include "./asp.h"
#include "../utils/widget.h"
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

    }
}
