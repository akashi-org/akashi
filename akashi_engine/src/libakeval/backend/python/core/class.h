#pragma once

#include "./object.h"

#include <libakcore/path.h>
#include <libakcore/memory.h>

#include <string_view>

// forward declaration
#ifndef PyObject_HEAD
struct _object;
typedef _object PyObject;
#endif

namespace akashi {
    namespace eval {

        class PythonObject;
        class PythonModule;

        // template <typename _InstType, typename... _Args>
        // class PythonClass {
        //   public:
        //     explicit PythonClass(core::borrowed_ptr<PythonModule> p_module,
        //                          std::string_view class_name);
        //     virtual ~PythonClass(void) noexcept;

        //     virtual _InstType init(const _Args&...) = 0;

        //     core::borrowed_ptr<PythonObject> class_obj(void) const {
        //         return core::borrowed_ptr(m_class_obj);
        //     }

        //   protected:
        //     core::owned_ptr<PythonObject> m_class_obj;
        // };

        class PythonInstance {
          public:
            explicit PythonInstance(core::owned_ptr<PythonObject> inst_obj) {
                m_inst_obj = std::move(inst_obj);
            };

            virtual ~PythonInstance(void){};
            core::borrowed_ptr<PythonObject> inst_obj(void) const {
                return core::borrowed_ptr(m_inst_obj);
            }

          protected:
            core::owned_ptr<PythonObject> m_inst_obj;
        };
    }
}
