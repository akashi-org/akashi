#include "./import.h"

#include "./list.h"
#include "./module.h"

#include <libakcore/logger.h>
#include <libakcore/memory.h>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <vector>
#include <string>

using namespace akashi::core;

namespace akashi {
    namespace eval {

        core::owned_ptr<PythonModule> from_import_module(const char* from,
                                                         std::vector<const char*> module_names) {
            // https://stackoverflow.com/a/47892218
            auto pymodule =
                PyImport_ImportModuleEx(from, nullptr, nullptr, make_list(module_names)->get_raw());
            if (!pymodule) {
                PyErr_Print();
                AKLOG_ERRORN("from_import_module(): Failed to import");
                return nullptr;
            }
            return make_owned<PythonModule>(pymodule);
        }

        core::owned_ptr<PythonModule> import_module(const char* module_name) {
            auto pymodule = PyImport_ImportModule(module_name);
            if (!pymodule) {
                PyErr_Print();
                AKLOG_ERRORN("import_module(): Failed to import");
                return nullptr;
            }
            return make_owned<PythonModule>(pymodule);
        }

    }
}
