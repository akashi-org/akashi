#pragma once

#include <libakcore/class.h>
#include <libakcore/memory.h>

// forward declaration
#ifndef PyObject_HEAD
struct _object;
typedef _object PyObject;
#endif

namespace akashi {
    namespace eval {

        class PythonObject {
            AK_FORBID_COPY(PythonObject)
          public:
            explicit PythonObject(PyObject* pyo, bool owned = true) : m_pyo(pyo), m_owned(owned){};
            virtual ~PythonObject(void) noexcept;
            // [XXX] DO Not delete this ptr!
            PyObject* get_raw(void) const { return m_pyo; }

            void set_owned(bool owned) { m_owned = owned; }

            bool owned(void) const { return m_owned; }

            core::owned_ptr<PythonObject> attr(const char* attr_name) const;

            bool has_attr(const char* attr_name) const;

            bool is_none(void) const;

          protected:
            PyObject* m_pyo;
            bool m_owned = true;
        };

    }
}
