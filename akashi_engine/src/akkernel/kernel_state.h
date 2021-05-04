#pragma once

#include <string>
#include <mutex>
#include <functional>

namespace akashi {
    namespace kernel {

        class KernelState final {
          public:
            struct Prop {
                int seq_process_error_exit = 0;
                std::string config_jstr;
                std::string renderer_path;
                uint32_t port = 0;
                int max_wait_ms_renderer_wakeup = 5000;
            };
            using Transformer = std::function<void(Prop& new_prop)>;

            explicit KernelState() = default;
            virtual ~KernelState() = default;

            const KernelState::Prop& prop(void) {
                {
                    std::lock_guard<std::mutex> lock(m_prop.mtx);
                    return m_prop.value;
                }
            }

            void transform_prop(const KernelState::Transformer& transformer) {
                {
                    std::lock_guard<std::mutex> lock(m_prop.mtx);
                    transformer(m_prop.value);
                }
            }

          private:
            struct {
                KernelState::Prop value;
                std::mutex mtx;
            } m_prop;
        };

    }
}
