#pragma once

#include "./class.h"
#include <string>
#include <cstdlib>
#include <vector>

namespace akashi {
    namespace core {

        class owned_string final {
            AK_FORBID_COPY(owned_string)
          public:
            explicit owned_string(const std::string& str) noexcept;
            explicit owned_string(const char* str) noexcept;

            virtual ~owned_string(void) noexcept;

            owned_string(owned_string&& o) noexcept;

            owned_string& operator=(owned_string&& o) noexcept;

            const std::string to_std_string() const;

            const char* to_str() const;

            char* to_cloned_str() const;

          private:
            char* clone(const std::string& str) const;

            void destroy() noexcept;

          private:
            char* m_str = nullptr;
        };

        bool ends_with(const std::string& str, const std::string& pat);

        std::vector<std::string> split_by(const std::string& in_str, const std::string& delim);

    }
}
