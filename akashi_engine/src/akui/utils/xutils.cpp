#include "./xutils.h"

#include <libakcore/logger.h>

#include <QWidget>
#include <QX11Info>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

namespace akashi {
    namespace ui {

        struct AKXDisplay {
            ::Display* native_disp = nullptr;
        };

        struct AKXWindow {
            XID native_winid = None;
        };

        namespace detail {

            std::string get_x_error_text(AKXDisplay* disp, int err_code) {
                constexpr const int length = 256;
                char err_text[length] = {0};
                XGetErrorText(disp->native_disp, err_code, err_text, length);
                return std::string(err_text);
            }

        }

        AKXDisplay* get_x_display() { return new AKXDisplay{QX11Info::display()}; }

        void free_x_display_wrapper(AKXDisplay* disp) {
            if (disp) {
                delete disp;
            }
        }

        AKXWindow* get_current_active_window(AKXDisplay* disp) {
            ::Atom act_reqtype;
            ::Window* act_win = nullptr;
            int act_fmt = -1;
            unsigned long nitems = 0;
            unsigned long left = 0;

            auto win_atom = ::XInternAtom(disp->native_disp, "_NET_ACTIVE_WINDOW", false);

            auto status = ::XGetWindowProperty(
                disp->native_disp, DefaultRootWindow(disp->native_disp), win_atom, 0, 4, false,
                XA_WINDOW, &act_reqtype, &act_fmt, &nitems, &left, (unsigned char**)&act_win);

            if (status != Success) {
                AKLOG_ERROR("{}", detail::get_x_error_text(disp, status).c_str());
                if (act_win) {
                    XFree(act_win);
                }
                return nullptr;
            }

            assert(act_reqtype == XA_WINDOW);
            assert(nitems == 1);

            if (!act_win) {
                AKLOG_ERRORN("No active window found");
                return nullptr;
            }

            XID tar_winid = *act_win;
            XFree(act_win);

            return new AKXWindow{tar_winid};
        }

        void free_x_window_wrapper(AKXWindow* win) {
            if (win) {
                delete win;
            }
        }

        void set_transient(AKXDisplay* disp, AKXWindow* parent_win, const QWidget* widget) {
            ::XSetTransientForHint(disp->native_disp, widget->winId(), parent_win->native_winid);
        }

        void raise_window(AKXDisplay* disp, AKXWindow* win) {
            XEvent x_evt;
            x_evt.xclient.type = ClientMessage;
            x_evt.xclient.window = win->native_winid;
            x_evt.xclient.message_type =
                ::XInternAtom(disp->native_disp, "_NET_ACTIVE_WINDOW", False);
            x_evt.xclient.format = 32;
            x_evt.xclient.data.l[0] = 1;
            // Specify timestamp
            // For now, we set it to zero, but maybe we should set x11's server time instead
            x_evt.xclient.data.l[1] = 0;
            x_evt.xclient.data.l[2] = 0;
            x_evt.xclient.data.l[3] = 0;
            x_evt.xclient.data.l[4] = 0;

            ::XSendEvent(disp->native_disp, DefaultRootWindow(disp->native_disp), False,
                         SubstructureRedirectMask, &x_evt);
            ::XMapRaised(disp->native_disp, win->native_winid);
        }

    }
}
