#pragma once

#include "../../core/module.h"
#include "../../core/class.h"

#include <libakcore/path.h>
#include <libakcore/memory.h>

#include <string_view>

namespace akashi {

    namespace core {
        struct Fraction;
        class Rational;
    }
    namespace eval {

        class PythonObject;

        namespace lib::akashi_core {

            class InitModule;
            class TimeModule final : public PythonModule {
              public:
                explicit TimeModule(PyObject* pymodule, core::borrowed_ptr<InitModule> init_module);
                virtual ~TimeModule();

                core::owned_ptr<PythonInstance> Second(int64_t a, int64_t b);

                core::owned_ptr<PythonInstance> Second(const core::Fraction& frac);

                core::owned_ptr<PythonInstance> Second(const core::Rational& rat);

              private:
                core::borrowed_ptr<InitModule> m_init_module;
                core::owned_ptr<PythonObject> m_second_cls;
            };

        };

    }

}
