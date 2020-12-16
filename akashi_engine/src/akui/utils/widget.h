#pragma once

#include <functional>
#include <string>

class QWidget;
class QObject;

namespace akashi {
    namespace ui {

        std::string get_abs_widget_name(const QWidget* widget);

        void ensure_widget_name(const QWidget* widget);

        bool is_widget(const QObject* obj);

        void walk_widgets(const QWidget* root,
                          const std::function<void(const QWidget* widget)>& cb);

    }
}
