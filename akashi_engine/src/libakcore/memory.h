#pragma once

#include <memory>
#include <stdexcept>

namespace akashi {
    namespace core {

        template <class T, class D = std::default_delete<T>>
        class borrowed_ptr final {
          public:
            explicit borrowed_ptr(const std::unique_ptr<T, D>& ptr) : m_ptr(ptr.get()){};
            explicit borrowed_ptr(T* ptr) : m_ptr(ptr){};
            ~borrowed_ptr() = default;

            inline T* operator->() {
#ifndef NDEBUG
                this->validate();
#endif
                return m_ptr;
            }
            inline const T* operator->() const {
#ifndef NDEBUG
                this->validate();
#endif
                return m_ptr;
            }
            inline T& operator*() {
#ifndef NDEBUG
                this->validate();
#endif
                return *m_ptr;
            }
            inline const T& operator*() const {
#ifndef NDEBUG
                this->validate();
#endif
                return *m_ptr;
            }

            explicit operator bool() const noexcept { return m_ptr != nullptr; }

          private:
            void validate() const noexcept(false) {
                if (!m_ptr) {
                    throw std::runtime_error("borrowed_ptr::validate(): ptr is null");
                }
            }

          private:
            T* m_ptr;
        };

        template <class T, class D = std::default_delete<T>>
        using owned_ptr = std::unique_ptr<T, D>;

        template <typename T, typename... Args>
        inline owned_ptr<T> make_owned(Args&&... args) {
            return std::make_unique<T>(std::forward<Args>(args)...);
        }

    }

}
