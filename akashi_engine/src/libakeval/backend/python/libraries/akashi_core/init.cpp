#include "./init.h"
#include "./kron.h"
#include "./kron_utils.h"
#include "./time.h"

#include "../../core/module.h"
#include "../../core/object.h"
#include "../../core/import.h"

#include <libakcore/path.h>
#include <libakcore/memory.h>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

using namespace akashi::core;

namespace akashi {
    namespace eval {

        namespace lib::akashi_core {

            core::owned_ptr<InitModule> InitModule::from(void) {
                auto modules = from_import_module("akashi_core", {"time", "kron", "kron_utils"});
                return make_owned<InitModule>(std::move(modules));
            }

            InitModule::InitModule(core::owned_ptr<PythonModule> pymodule)
                : PythonModule(pymodule->get_module()->get_raw()) {
                m_init_module = std::move(pymodule);
                m_time_module = make_owned<TimeModule>(
                    m_init_module->get_module()->attr("time")->get_raw(), borrowed_ptr(this));
                m_kron_module = make_owned<KronModule>(
                    m_init_module->get_module()->attr("kron")->get_raw(), borrowed_ptr(this));
                m_kron_utils_module = make_owned<KronUtilsModule>(
                    m_init_module->get_module()->attr("kron_utils")->get_raw(), borrowed_ptr(this));
            }

            InitModule::~InitModule() {}

        }

    }
}
