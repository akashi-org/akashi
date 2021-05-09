#pragma once

class QWidget;

namespace akashi {
    namespace ui {

        struct AKXDisplay;
        struct AKXWindow;

        AKXDisplay* get_x_display();

        void free_x_display_wrapper(AKXDisplay* disp);

        AKXWindow* get_current_active_window(AKXDisplay* disp);

        void free_x_window_wrapper(AKXWindow* win);

        void set_transient(AKXDisplay* disp, AKXWindow* parent_win, const QWidget* widget);

        void raise_window(AKXDisplay* disp, AKXWindow* win);

    }
}
