#include "./asp.h"
#include "../components/PlayerWidget/PlayerWidget.h"

#include <libakserver/akserver.h>

#include <csignal>
#include <unistd.h>

#include <QWidget>

using namespace akashi::core;

namespace akashi {
    namespace ui {

        ASPGeneralAPIImpl::ASPGeneralAPIImpl(QWidget* root) : m_root(root) {
            m_player = m_root->findChild<PlayerWidget*>("player_widget");
        }

        bool ASPGeneralAPIImpl::eval(const std::string& file_path, const std::string& elem_name) {
            return QMetaObject::invokeMethod(
                m_player, [&]() { m_player->inline_eval(file_path, elem_name); },
                Qt::BlockingQueuedConnection);
        }

        bool ASPGeneralAPIImpl::terminate(void) {
            kill(getpid(), SIGTERM);
            return true;
        }

    }
}
