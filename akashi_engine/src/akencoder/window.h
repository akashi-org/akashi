#pragma once

#include <libakcore/memory.h>

namespace akashi {
    namespace encoder {

        struct PrivWindow;

        class Window final {
          public:
            explicit Window(int msaa);
            virtual ~Window();

          public:
            static void* get_proc_address(const char* name);
            static void* egl_get_proc_address(const char* name);

          private:
            core::owned_ptr<PrivWindow> m_window;
        };

    }
}
