#pragma once

#include <libakcore/memory.h>

// forward declaration
#ifndef PyObject_HEAD
struct _object;
typedef _object PyObject;
#endif

#include <vector>
#include <string>

namespace akashi {
    namespace eval {

        class PythonModule;

        core::owned_ptr<PythonModule> from_import_module(const char* from,
                                                         std::vector<const char*> module_names);

        core::owned_ptr<PythonModule> import_module(const char* module_name);

    }
}
