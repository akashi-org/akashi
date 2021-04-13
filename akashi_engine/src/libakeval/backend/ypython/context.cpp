#include "./context.h"

#include "../../item.h"

#include <libakstate/akstate.h>
#include <libakcore/path.h>
#include <libakcore/memory.h>
#include <libakcore/logger.h>
#include <libakcore/element.h>
#include <libakcore/rational.h>
#include <libakwatch/item.h>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>
#include <condition_variable>
#include <mutex>
#include <thread>

#include <libakcore/perf.h>

#include <stdexcept>

using namespace akashi::core;

#include <pybind11/embed.h>
namespace py = pybind11;

namespace akashi {
    namespace eval {

        struct PyBind11Module {
            pybind11::module_ mod;
        };

        YPyEvalContext::YPyEvalContext(core::borrowed_ptr<state::AKState> state)
            : EvalContext(state), m_state(state) {
            AKLOG_DEBUGN("YPyEvalContext init");

            py::initialize_interpreter();

            auto config = this->config();

            // [XXX] When using pybind11, make sure that `__init__.py` does not exist in the
            // directory where the entry file exists. Otherwise module importing will mess up.
            auto sys_path = py::module_::import("sys").attr("path");

            sys_path.cast<py::list>().append(config.include_dir.to_abspath().to_cloned_str());
            if (std::getenv("AK_CORELIB_PATH")) {
                sys_path.cast<py::list>().insert(
                    0, core::Path(std::getenv("AK_CORELIB_PATH")).to_abspath().to_cloned_str());
            }

            // version check
            auto res = py::module_::import("akashi_core2").attr("utils").attr("version_check")();
            if (!res.cast<py::tuple>()[0].cast<bool>()) {
                AKLOG_WARN("{}", res.cast<py::tuple>()[1].cast<std::string>().c_str());
            }

            this->load_module(config.entry_path, config.include_dir);
            this->register_deps_module(config.entry_path, config.include_dir);
        };

        YPyEvalContext::~YPyEvalContext(void) noexcept { this->exit(); };

        // [TODO] since this is called from outside the eval thread, maybe we should use GIL?
        void YPyEvalContext::exit(void) {
            if (!m_exited) {
                m_modules.clear();
                py::finalize_interpreter();
                m_exited = true;
            }
        };

        core::FrameContext YPyEvalContext::eval_kron(const char* module_path, const KronArg& arg){};

        std::vector<core::FrameContext> YPyEvalContext::eval_krons(const char* module_path,
                                                                   const core::Rational& start_time,
                                                                   const int fps,
                                                                   const core::Rational& duration,
                                                                   const size_t length) {}

        core::RenderProfile YPyEvalContext::render_prof(const char* module_path) {}

        void YPyEvalContext::reload(const std::vector<watch::WatchEvent>& events) {}

        const state::EvalConfig& YPyEvalContext::config(void) {
            std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
            return m_state->m_prop.eval_state.config;
        }

        bool YPyEvalContext::load_module(const core::Path& module_path,
                                         const core::Path& include_dir) {
            auto mod_ptr = core::make_owned<PyBind11Module>();
            mod_ptr->mod = py::module_::import(
                module_path.to_relpath(include_dir).to_pymodule_name().to_str());
            m_modules.insert({module_path.to_abspath().to_cloned_str(), std::move(mod_ptr)});

            AKLOG_DEBUG("Loaded Python module: {}", module_path.to_str());

            return true;
        }

        bool YPyEvalContext::register_deps_module(const core::Path& entry_path,
                                                  const core::Path& include_dir) {
            for (const auto& mod_dict :
                 py::module_::import("sys").attr("modules").cast<py::dict>()) {
                auto mod = mod_dict.second;
                if (!py::hasattr(mod, "__file__")) {
                    continue;
                }
                auto mod_fpath = py::str(mod.attr("__file__"));
                if (!py::isinstance<py::str>(mod_fpath)) {
                    continue;
                }
                auto mod_fpath_str = core::Path(mod_fpath.cast<std::string>());
                if ((mod_fpath_str != entry_path) && (mod_fpath_str.is_subordinate(include_dir))) {
                    this->load_module(mod_fpath_str, include_dir);
                }
            }
            return true;
        }

    }
}
