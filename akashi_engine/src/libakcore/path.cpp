#include "./path.h"

#include "./logger.h"

#include <filesystem>
#include <string>
#include <algorithm>

#include <boost/algorithm/string.hpp>

namespace akashi {
    namespace core {

        const Path Path::to_abspath() const {
            return Path(std::filesystem::weakly_canonical(m_std_path));
        }

        const Path Path::to_relpath(const Path base) const {
            return Path(
                this->to_abspath().m_std_path.lexically_relative(base.to_abspath().m_std_path));
        }

        bool Path::is_subordinate(const Path base) const {
            return this->to_abspath().m_std_path.string().rfind(
                       base.to_abspath().m_std_path.string()) == 0;
        }

        const Path Path::to_pymodule_name() const {
            auto dot_pos = m_std_path.string().find_last_of(".");
            auto name = m_std_path.string().substr(0, dot_pos);
            boost::replace_all(name, "/", ".");
            return Path(name);
        }

        const Path Path::to_dirpath() const { return Path(m_std_path.parent_path()); }

        const char* Path::to_cloned_str() const { return this->clone(m_std_path); }

        const char* Path::to_str() const { return m_std_path.c_str(); }

        const Path Path::to_stem() const { return Path(m_std_path.stem()); }

        bool Path::operator==(const Path& other) const {
            return std::filesystem::equivalent(this->m_std_path, other.m_std_path);
        }

        bool Path::operator!=(const Path& other) const { return !(*this == other); }

        const char* Path::clone(const std_path_t& std_path) const {
            auto path_str = std_path.string();
            char* res_path = static_cast<char*>(malloc(path_str.length() + 1));
            if (res_path == nullptr) {
                AKLOG_ERRORN("Failed to allocate memory");
                return nullptr;
            }

            auto len = path_str.copy(res_path, path_str.length(), 0);
            res_path[len] = '\0';

            return res_path;
        }

    }
}
