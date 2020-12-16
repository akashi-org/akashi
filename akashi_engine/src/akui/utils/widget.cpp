#include "./widget.h"

#include <libakcore/logger.h>

#include <QWidget>

#include <string>
#include <list>
#include <queue>
#include <functional>

using namespace akashi::core;

namespace akashi {
    namespace ui {

        std::string get_abs_widget_name(const QWidget* widget) {
            std::list<const char*> names;
            auto child = widget;
            while (child) {
                names.push_front(child->metaObject()->className());
                child = child->parentWidget();
            }

            std::string class_name = "";
            for (auto it = names.begin(); it != names.end(); ++it) {
                class_name += *it;
                if (std::next(it) != names.end()) {
                    class_name += "/";
                }
            }

            return class_name;
        }

        void ensure_widget_name(const QWidget* widget) {
            auto widget_name = widget->objectName();
            auto abs_widget_name = get_abs_widget_name(widget);
            if (widget_name.length() == 0) {
                AKLOG_WARN("ensure_widget_name(): widget name is not set for {}",
                           abs_widget_name.c_str());
            }
        }

        bool is_widget(const QObject* obj) { return obj && obj->inherits("QWidget"); }

        void walk_widgets(const QWidget* root,
                          const std::function<void(const QWidget* widget)>& cb) {
            std::queue<const QWidget*> widgets;
            widgets.push(root);

            while (!widgets.empty()) {
                auto widget = widgets.front();
                cb(widget);
                widgets.pop();
                for (auto it = widget->children().begin(); it != widget->children().end(); ++it) {
                    if (is_widget(*it)) {
                        widgets.push(static_cast<QWidget*>(*it));
                    }
                }
            }
        }

    }
}
