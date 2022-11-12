#include "./akdebug.h"
#include "./glc.h"

#include <libakcore/logger.h>

#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

using namespace akashi::core;

namespace akashi {
    namespace debug {

        namespace priv {
            static GLFWwindow* create_window(const DebugUIConfig& config) {
                glfwInit();
                glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
                glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
                glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

                glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
                // on linux/x11 system, force egl over glx
                glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);

                glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

                glfwWindowHint(GLFW_SAMPLES, 1);

                auto window = glfwCreateWindow(config.init_win_width, config.init_win_height,
                                               "Akashi Debug Window", NULL, NULL);

                if (window == nullptr) {
                    AKLOG_ERRORN("Failed to create GLFW window");
                    glfwTerminate();
                    return nullptr;
                }

                glfwMakeContextCurrent(window);
                glfwSwapInterval(1);

                // if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
                //     AKLOG_ERRORN("Failed to load OpenGL functions");
                //     glfwDestroyWindow(window);
                //     glfwTerminate();
                //     return nullptr;
                // }

                return window;
            }

            static void before_render(GLFWwindow* window) {
                int fb_width = 0;
                int fb_height = 0;
                glfwGetFramebufferSize(window, &fb_width, &fb_height);

                glViewport(0.0, 0.0, fb_width, fb_height);
                glClearColor(30 / 255.0, 30 / 255.0, 30 / 255.0, 1);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            }

        };

        bool ListWidget::init(const DebugUIConfig& config) {
            m_config = config;
            return true;
        }

        void ListWidget::destroy() {
            {
                std::lock_guard<std::mutex> lock(m_thread_alive.mtx);
                m_thread_alive.value = false;
            }
            if (m_th) {
                m_th->join();
                delete m_th;
                m_th = nullptr;
            }
        }

        void ListWidget::add_field(const std::string& key, const std::string& initial_value) {
            {
                std::lock_guard<std::mutex> lock(m_common_mtx);
                m_fields[key] = initial_value;
                m_need_render = true;
            }
        }

        void ListWidget::update_field(const std::string& key, const std::string& value) {
            {
                std::lock_guard<std::mutex> lock(m_common_mtx);

                if (m_fields.find(key) == m_fields.end()) {
                    AKLOG_ERROR("key({}) not found", key.c_str());
                    return;
                }

                m_fields[key] = value;
                m_need_render = true;
            }
        }

        void ListWidget::run() {
            {
                std::lock_guard<std::mutex> lock(m_thread_alive.mtx);
                m_thread_alive.value = true;
            }
            m_th = new std::thread(&ListWidget::render_thread, this);
        }

        bool ListWidget::need_render() {
            {
                std::lock_guard<std::mutex> lock(m_common_mtx);
                return m_need_render;
            }
        }

        void ListWidget::set_need_render(bool value) {
            {
                std::lock_guard<std::mutex> lock(m_common_mtx);
                m_need_render = value;
            }
        }

        const std::map<std::string, std::string>& ListWidget::fields() {
            {
                std::lock_guard<std::mutex> lock(m_common_mtx);
                return m_fields;
            }
        }

        void ListWidget::render_thread(ListWidget* self) {
            AKLOG_INFON("Debug Window render thread started");

            auto window = priv::create_window(self->m_config);
            if (!window) {
                AKLOG_INFON("Debug Window render thread ended");
                return;
            }
            glfwSetWindowUserPointer(window, self);
            glfwSetFramebufferSizeCallback(
                window, [](GLFWwindow* win_, int /*fb_width*/, int /*fb_height*/) {
                    auto self_ = reinterpret_cast<ListWidget*>(glfwGetWindowUserPointer(win_));
                    if (self_) {
                        self_->set_need_render(true);
                    }
                });

            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            (void)io;
            io.IniFilename = nullptr;
            ImGui::StyleColorsDark();
            ImGui_ImplGlfw_InitForOpenGL(window, true);
            ImGui_ImplOpenGL3_Init("#version 420 core");

            double init_time = glfwGetTime();
            double last_time = init_time;
            double frame_per_sec = (1.0 / 60);
            int imgui_init_dummy = 2;
            while (!glfwWindowShouldClose(window)) {
                {
                    std::lock_guard<std::mutex> lock(self->m_thread_alive.mtx);
                    if (!self->m_thread_alive.value) {
                        break;
                    }
                }

                glfwPollEvents();

                auto cur_time = glfwGetTime();
                auto delta_time = cur_time - last_time;
                if (auto diff = frame_per_sec - delta_time; diff > 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds((long)(diff * 1000)));
                    continue;
                }

                // AKLOG_DEBUG("delta_time: {}", delta_time);

                if (self->need_render() || imgui_init_dummy > 0) {
                    priv::before_render(window);

                    {
                        ImGui_ImplOpenGL3_NewFrame();
                        ImGui_ImplGlfw_NewFrame();
                        ImGui::NewFrame();

                        {
                            auto win_flags =
                                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize |
                                ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration |
                                ImGuiWindowFlags_NoInputs;

                            ImGui::Begin("Debug Frame", nullptr, win_flags);

                            for (auto fld : self->fields()) {
                                ImGui::Text("%s: %s", fld.first.c_str(), fld.second.c_str());
                            }
                            // ImGui::Text("a: %f", 1.0);
                            // ImGui::Text("b: %f", 2.11);

                            ImGui::End();
                        }
                        ImGui::Render();
                        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
                        if (imgui_init_dummy > 0) {
                            imgui_init_dummy -= 1;
                        }
                    }

                    glfwSwapBuffers(window);
                    self->set_need_render(false);
                }
                last_time = cur_time;
            }

            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();

            glfwDestroyWindow(window);
            glfwTerminate();

            AKLOG_INFON("Debug Window render thread ended");
        }
    }
}
