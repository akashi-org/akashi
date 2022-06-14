#include "./asp.h"
#include "../utils/widget.h"

#include <libakserver/akserver.h>

#include <QWidget>
#include <QtTest/QtTest>

#include <vector>

namespace akashi {
    namespace ui {

        std::vector<std::string> ASPGUIAPIImpl::get_widgets(void) {
            std::vector<std::string> widget_names;

            walk_widgets(m_root, [&widget_names](const QWidget* widget) {
                auto widget_name = widget->objectName();
                if (widget_name.length() > 0) {
                    widget_names.push_back(widget_name.toUtf8().constData());
                }
            });

            return widget_names;
        };

        bool ASPGUIAPIImpl::click(const std::string& widget_name) {
            auto widget = m_root->findChild<QWidget*>(QString::fromUtf8(widget_name.c_str()));
            if (!widget) {
                return false;
            }
            // [XXX] DO NOT directly call QTest::mouseClick!
            return QMetaObject::invokeMethod(
                m_root, [widget]() { QTest::mouseClick(widget, Qt::LeftButton); },
                Qt::BlockingQueuedConnection);
            // if you prefer non-blocking execution, change Qt::BlockingQueuedConnection to
            // Qt::QueuedConnection
        };

    }
}
