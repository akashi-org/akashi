#pragma once

namespace akashi {
    namespace graphics {

        template <class T>
        class ResourceLoader {
          public:
            static inline T& GetInstance() {
                static T s_instance;
                return s_instance;
            }

          protected:
            ResourceLoader() = default;
            virtual ~ResourceLoader() = default;

          private:
            /* ResourceLoader(const ResourceLoader&) = delete; */
            /* ResourceLoader(ResourceLoader&&) = delete; */
            /* ResourceLoader& operator=(const ResourceLoader&) = delete; */
            /* ResourceLoader& operator=(ResourceLoader&&) = delete; */
        };

    }
}
