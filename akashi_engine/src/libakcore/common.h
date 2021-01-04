#pragma once

#include <map>
#include <string>
#include <vector>

namespace akashi {
    namespace core {
        namespace json {
            namespace optional {

                template <class T>
                using type = std::map<std::string, T>;
                constexpr static char attr_name[] = "value";

                template <class T>
                T unwrap(optional::type<T>& optional_v) {
                    return optional_v.at(optional::attr_name);
                }

                template <class T>
                T unwrap(optional::type<T> const& optional_v) {
                    return optional_v.at(optional::attr_name);
                }

                template <class T>
                T unwrap_or(optional::type<T>& optional_v) {
                    return optional_v[optional::attr_name];
                }

                template <class T>
                T unwrap_or(optional::type<T> const& optional_v) {
                    return optional_v[optional::attr_name];
                }

                template <class T>
                bool is_none(optional::type<T>& optional_v) {
                    return optional_v.count(optional::attr_name) == 0;
                }

                template <class T>
                bool is_none(optional::type<T> const& optional_v) {
                    return optional_v.count(optional::attr_name) == 0;
                }

            }
        }
    }
}
