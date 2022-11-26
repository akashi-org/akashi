#pragma once

#include <libakcore/memory.h>
#include <map>
#include <thread>
#include <mutex>

namespace akashi {
    namespace debug {

        struct DebugUIConfig {
            int init_win_width = 640;
            int init_win_height = 360;
        };

        class ListWidget final {
          public:
            static inline ListWidget& get() {
                static ListWidget s_instance;
                return s_instance;
            }

          private:
            explicit ListWidget() = default;
            virtual ~ListWidget() = default;

          public:
            bool init(const DebugUIConfig& config);

            void destroy();

            void add_field(const std::string& key, const std::string& initial_value);

            void update_field(const std::string& key, const std::string& value);

            void run();

          private:
            bool need_render();

            void set_need_render(bool value);

            const std::map<std::string, std::string>& fields();

            static void render_thread(ListWidget* self);

          private:
            DebugUIConfig m_config;

            std::map<std::string, std::string> m_fields;
            bool m_need_render = true;
            std::mutex m_common_mtx;

            std::thread* m_th = nullptr;
            struct {
                bool value = false;
                std::mutex mtx;
            } m_thread_alive;
        };

    }
}
