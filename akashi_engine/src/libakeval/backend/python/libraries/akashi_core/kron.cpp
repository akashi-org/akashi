#include "./kron.h"
#include "./init.h"
#include "./time.h"

#include "../../core/object.h"
#include "../../core/module.h"
#include "../../core/class.h"
#include "../../core/tuple.h"
#include "../../../../item.h"

#include <libakcore/path.h>
#include <libakcore/memory.h>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

using namespace akashi::core;

namespace akashi {
    namespace eval {

        namespace lib::akashi_core {

            KronModule::KronModule(PyObject* pymodule, core::borrowed_ptr<InitModule> init_module)
                : PythonModule(pymodule), m_init_module(init_module) {
                m_kron_args_cls = m_module->attr("KronArgs");
            }

            KronModule::~KronModule() {}

            core::owned_ptr<PythonInstance> KronModule::KronArgs(const KronArg& arg) {
                auto sec = m_init_module->time()->Second(arg.play_time.num(), arg.play_time.den());
                sec->inst_obj()->set_owned(false);

                return make_owned<PythonInstance>(make_owned<PythonObject>(PyObject_CallObject(
                    m_kron_args_cls->get_raw(),
                    make_tuple(sec->inst_obj()->get_raw(), arg.fps)->get_raw())));
            }

        }

    }

}
