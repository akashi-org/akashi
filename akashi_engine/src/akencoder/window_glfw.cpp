#include "./window.h"

#include <libakcore/memory.h>
#include <libakcore/logger.h>
#include <libakstate/akstate.h>

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_X11
// #define GLFW_EXPOSE_NATIVE_WAYLAND
#define GLFW_EXPOSE_NATIVE_EGL
#include <GLFW/glfw3native.h>

#include <EGL/egl.h>

using namespace akashi::core;

namespace akashi {
    namespace encoder {

        struct PrivWindow {
            GLFWwindow* window = nullptr;
        };

        Window::Window() {
            glfwInit();
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

            glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
            // on linux/x11 system, force egl over glx
            glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);

            // glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
            // glfwWindowHint(GLFW_CONTEXT_NO_ERROR, GLFW_TRUE);

            glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

            m_window = make_owned<PrivWindow>();

            m_window->window = glfwCreateWindow(640, 480, "", NULL, NULL);

            glfwMakeContextCurrent(m_window->window);
        }

        Window::~Window() {
            if (this->m_window && this->m_window->window) {
                glfwDestroyWindow(this->m_window->window);
                glfwTerminate();
            }
        }

        void* Window::get_proc_address(const char* name) {
            return reinterpret_cast<void*>(glfwGetProcAddress(name));
        }

        void* Window::egl_get_proc_address(const char* name) {
            // [TODO] add checks for validity of egl?
            return reinterpret_cast<void*>(eglGetProcAddress(name));
        }

    }
}
