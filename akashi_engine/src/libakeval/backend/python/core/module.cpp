#include "./module.h"

#include "./object.h"
#include "./string.h"
#include "../../../item.h"

#include <libakcore/path.h>
#include <libakcore/logger.h>
#include <libakcore/memory.h>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <stdexcept>
#include <string_view>

using namespace akashi::core;

namespace akashi {
    namespace eval {

        PythonModule::PythonModule(core::Path path) noexcept(false) {
            m_path = path;

            // [XXX] use of temporary string is safe because PyImport_Import does not save pName
            // in any states
            auto pName =
                PythonObject(PyUnicode_DecodeFSDefault(m_path.to_pymodule_name().to_str()));
            /* Error checking of pName left out */

            m_module = make_owned<PythonObject>(PyImport_Import(pName.get_raw()));
            if (!m_module) {
                PyErr_Print();
                AKLOG_ERROR("PythonModule::PythonModule() Failed to load \"{}\"", m_path.to_str());
                throw std::runtime_error("");
            }
        };

        PythonModule::PythonModule(PyObject* pymodule) noexcept(false) {
            auto mod_path = PyModule_GetFilenameObject(pymodule);
            if (!mod_path) {
                PyErr_Print();
                AKLOG_ERROR("PythonModule::PythonModule() Failed to load \"{}\"", m_path.to_str());
                throw std::runtime_error("");
            }
            // [TODO] possible lifetime issues
            m_path = Path(PythonString(mod_path).char_p());

            m_module = make_owned<PythonObject>(pymodule);
            if (!m_module) {
                PyErr_Print();
                AKLOG_ERROR("PythonModule::PythonModule() Failed to load \"{}\"", m_path.to_str());
                throw std::runtime_error("");
            }
        }

        PythonModule::~PythonModule(void) noexcept {};

        bool PythonModule::reload(void) {
            auto new_module = PyImport_ReloadModule(m_module->get_raw());
            if (!new_module) {
                if (PyErr_Occurred() != nullptr) {
                    PyErr_Print();
                }
                return false;
            }
            m_module.reset(new PythonObject(new_module));
            return true;
        };

    }
}
