#pragma once

// forward declaration
#ifndef PyObject_HEAD
struct _object;
typedef _object PyObject;
#endif

namespace akashi {
    namespace eval {

        class PythonString {
          public:
            explicit PythonString(PyObject* str, const char* encoding = "utf-8",
                                  const char* errors = "ignore");
            virtual ~PythonString(void) noexcept;

            const char* char_p(void) const;

          private:
            PyObject* m_bytes_obj = nullptr;
        };

    }
}
