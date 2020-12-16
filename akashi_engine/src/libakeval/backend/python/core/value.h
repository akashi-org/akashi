#pragma once

#include <libakcore/memory.h>

// forward declaration
#ifndef PyObject_HEAD
struct _object;
typedef _object PyObject;
#endif

namespace akashi {
    namespace eval {

        class PythonObject;

        class PythonValue {
          public:
            /* integers */
            static PythonValue from(long v, bool managed = true);
            static PythonValue from(unsigned long v, bool managed = true);
            static PythonValue from(long long v, bool managed = true);
            static PythonValue from(unsigned long long v, bool managed = true);

            /* float */
            static PythonValue from(double v, bool managed = true);

            /* string */
            static PythonValue from(const char* v, bool managed = true);

            /* bool */
            static PythonValue from(bool v, bool managed = true);

            /* PyObject */
            static PythonValue from(PyObject* pyo, bool managed = true);

          public:
            explicit PythonValue(PyObject* pyo, bool managed = true);
            virtual ~PythonValue(void){};

            virtual core::borrowed_ptr<PythonObject> value(void) const {
                return core::borrowed_ptr(m_value);
            }

            template <typename T>
            T to(void);

          private:
            core::owned_ptr<PythonObject> m_value;
        };

    }
}
