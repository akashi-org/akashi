#include "./string.h"

#include "./logger.h"

#include <string>
#include <cstdlib>

namespace akashi {
    namespace core {

        owned_string::owned_string(const std::string& str) noexcept { m_str = this->clone(str); }
        owned_string::owned_string(const char* str) noexcept { m_str = this->clone(str); }

        owned_string::~owned_string(void) noexcept { this->destroy(); }

        owned_string::owned_string(owned_string&& o) noexcept : m_str(o.m_str) {
            o.m_str = nullptr;
        }

        owned_string& owned_string::operator=(owned_string&& o) noexcept {
            this->destroy();
            m_str = o.m_str;
            o.m_str = nullptr;
            return *this;
        }

        const std::string owned_string::to_std_string() const { return std::string(m_str); }

        const char* owned_string::to_str() const { return m_str; }

        char* owned_string::to_cloned_str() const { return this->clone(std::string(m_str)); }

        char* owned_string::clone(const std::string& str) const {
            char* res_str = static_cast<char*>(malloc(str.length() + 1));
            if (res_str == nullptr) {
                AKLOG_ERRORN("owned_string::clone() Failed to allocate memory");
                return nullptr;
            }

            auto len = str.copy(res_str, str.length(), 0);
            res_str[len] = '\0';

            return res_str;
        }

        void owned_string::destroy() noexcept {
            if (m_str) {
                free(m_str);
            }
        }

    }
}
