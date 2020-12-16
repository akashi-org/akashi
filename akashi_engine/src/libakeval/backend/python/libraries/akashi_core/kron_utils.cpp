#include "./kron_utils.h"
#include "./init.h"
#include "./kron.h"
#include "./time.h"

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

            KronUtilsModule::KronUtilsModule(PyObject* pymodule,
                                             core::borrowed_ptr<InitModule> init_module)
                : PythonModule(pymodule), m_init_module(init_module) {
                m_get_render_profile_func = m_module->attr("get_render_profile");
                m_get_frame_contexts_func = m_module->attr("get_frame_contexts");
            }

            KronUtilsModule::~KronUtilsModule() {}

            core::owned_ptr<PythonObject>
            KronUtilsModule::get_render_profile(core::owned_ptr<PythonObject> kron,
                                                const KronArg& arg) {
                auto kron_args = m_init_module->kron()->KronArgs(arg);
                kron_args->inst_obj()->set_owned(false);

                kron->set_owned(false);
                Py_XINCREF(kron->get_raw()); // [TODO] should be executed within make_tuple?

                return make_owned<PythonObject>(PyObject_CallObject(
                    m_get_render_profile_func->get_raw(),
                    make_tuple(kron->get_raw(), kron_args->inst_obj()->get_raw())->get_raw()));
            }

            core::owned_ptr<PythonObject> KronUtilsModule::get_frame_contexts(
                core::owned_ptr<PythonObject> kron, const core::Rational& start_time, const int fps,
                const core::Rational& duration, const size_t length) {
                auto start_time_ = m_init_module->time()->Second(start_time);
                start_time_->inst_obj()->set_owned(false);

                auto duration_ = m_init_module->time()->Second(duration);
                duration_->inst_obj()->set_owned(false);

                kron->set_owned(false);
                Py_XINCREF(kron->get_raw()); // [TODO] should be executed within make_tuple?

                return make_owned<PythonObject>(PyObject_CallObject(
                    m_get_frame_contexts_func->get_raw(),
                    make_tuple(kron->get_raw(), start_time_->inst_obj()->get_raw(),
                               static_cast<long>(fps), duration_->inst_obj()->get_raw(),
                               static_cast<long>(length))
                        ->get_raw()));
            }

        }

    }

}
