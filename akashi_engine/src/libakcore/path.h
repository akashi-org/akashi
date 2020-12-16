#pragma once

#include <filesystem>

namespace akashi {
    namespace core {

        class Path final {
          private:
            using std_path_t = std::filesystem::path;

          public:
            explicit Path(const char* path) { m_std_path = path; }
            explicit Path(const std_path_t std_path) { m_std_path = std_path; }
            virtual ~Path(void) = default;
            // Path(const Path& self) { m_std_path = self.m_std_path; }

            const Path to_abspath() const;

            const Path to_relpath(const Path base) const;

            // [XXX] `base` must actually exist in the system, if not raises exception
            bool is_subordinate(const Path base) const;

            const Path to_pymodule_name() const;

            const Path to_dirpath() const;

            const char* to_cloned_str() const;

            const char* to_str() const;

            const Path to_stem() const;

            bool operator==(const Path& other) const;

            bool operator!=(const Path& other) const;

          private:
            const char* clone(const std_path_t& std_path) const;

          private:
            std_path_t m_std_path;
        };

    }
}
