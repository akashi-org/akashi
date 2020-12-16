#include "./time.h"
#include "./init.h"

#include "../../core/object.h"
#include "../../core/module.h"
#include "../../core/class.h"
#include "../../core/tuple.h"

#include <libakcore/path.h>
#include <libakcore/memory.h>
#include <libakcore/rational.h>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

using namespace akashi::core;

namespace akashi {
    namespace eval {

        namespace lib::akashi_core {

            TimeModule::TimeModule(PyObject* pymodule, core::borrowed_ptr<InitModule> init_module)
                : PythonModule(pymodule), m_init_module(init_module) {
                m_second_cls = m_module->attr("Second");
            }

            TimeModule::~TimeModule() {}

            core::owned_ptr<PythonInstance> TimeModule::Second(int64_t a, int64_t b) {
                return make_owned<PythonInstance>(make_owned<PythonObject>(
                    PyObject_CallObject(m_second_cls->get_raw(), make_tuple(a, b)->get_raw())));
            }

            core::owned_ptr<PythonInstance> TimeModule::Second(const Fraction& frac) {
                return this->Second(frac.num, frac.den);
            }

            core::owned_ptr<PythonInstance> TimeModule::Second(const core::Rational& rat) {
                return this->Second(rat.num(), rat.den());
            }

        }

    }

}
