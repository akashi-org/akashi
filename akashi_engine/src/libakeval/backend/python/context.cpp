#include "./context.h"

#include "../../item.h"
#include "./elem/elem_eval.h"

#include <libakstate/akstate.h>
#include <libakcore/path.h>
#include <libakcore/memory.h>
#include <libakcore/logger.h>
#include <libakcore/element.h>
#include <libakcore/rational.h>
#include <libakwatch/item.h>
#include <libakcore/perf.h>

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <stdexcept>

using namespace akashi::core;

#include <pybind11/embed.h>
namespace py = pybind11;

namespace akashi {
    namespace eval {

        struct PyBind11Module {
            pybind11::module_ mod;
        };

        PyEvalContext::PyEvalContext(core::borrowed_ptr<state::AKState> state)
            : EvalContext(state), m_state(state) {
            AKLOG_DEBUGN("PyEvalContext init");

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
            if (std::getenv("AK_LIBPROBE_PATH")) {
                py::module_::import("os").attr("environ")["AK_LIBPROBE_PATH"] =
                    std::getenv("AK_LIBPROBE_PATH");
            }

            if (std::getenv("AK_CORE_ARGS")) {
                py::module_::import("akashi_core")
                    .attr("args")
                    .attr("_register_argv")(std::getenv("AK_CORE_ARGS"));
            }

            // version check
            // auto res = py::module_::import("akashi_core").attr("utils").attr("version_check")();
            // if (!res.cast<py::tuple>()[0].cast<bool>()) {
            //     AKLOG_WARN("{}", res.cast<py::tuple>()[1].cast<std::string>().c_str());
            // }
        };

        void PyEvalContext::load(void) {
            if (m_loaded) {
                AKLOG_WARNN("Already loaded.");
            }

            auto config = this->config();
            this->load_module(config.entry_path, config.include_dir);
            this->register_deps_module(config.entry_path, config.include_dir);

            m_loaded = true;
        }

        PyEvalContext::~PyEvalContext(void) noexcept { this->exit(); };

        // [TODO] since this is called from outside the eval thread, maybe we should use GIL?
        void PyEvalContext::exit(void) {
            if (!m_exited) {
                m_modules.clear();
                m_gctx.reset();
                py::finalize_interpreter();
                m_exited = true;
            }
        };

        core::FrameContext PyEvalContext::eval_kron(const char* module_path, const KronArg& arg) {
            FrameContext frame_ctx;
            frame_ctx.pts = Rational{-1, 1};

            if (!m_gctx) {
                AKLOG_ERRORN("GlobalContext is null");
                return frame_ctx;
            }

            // timer timer;
            // timer.start();
            // aKLOG_DEBUGN("eval_kron() start");

            frame_ctx = local_eval(*m_gctx, arg);

            // timer.stop();
            // AKLOG_DEBUG("eval_kron() end, time: {} microseconds",
            //             timer.current_time_micro().to_decimal());

            return frame_ctx;
        };

        std::vector<core::FrameContext> PyEvalContext::eval_krons(const char* module_path,
                                                                  const core::Rational& start_time,
                                                                  const int fps,
                                                                  const core::Rational& duration,
                                                                  const size_t length) {
            std::vector<FrameContext> frame_ctxs = {};

            if (!m_gctx) {
                AKLOG_ERRORN("GlobalContext is null");
                return frame_ctxs;
            }

            for (size_t i = 0; i < length; i++) {
                auto frame_ctx =
                    local_eval(*m_gctx, {start_time + (Rational(i, 1) * Rational(1, fps)), fps});
                if (frame_ctx.pts <= duration) {
                    frame_ctxs.push_back(frame_ctx);
                }
            }

            auto diff = length - frame_ctxs.size();
            if (diff <= 0) {
                return frame_ctxs;
            } else {
                auto next_ctxs = this->eval_krons(module_path, Rational(0l), fps, duration, diff);
                frame_ctxs.insert(frame_ctxs.end(), next_ctxs.begin(), next_ctxs.end());
                return frame_ctxs;
            }
        }

        core::RenderProfile PyEvalContext::render_prof(const std::string& module_path,
                                                       const std::string& elem_name) {
            RenderProfile render_prof;
            render_prof.atom_profiles = {};

            auto it = m_modules.find(module_path);
            if (it == m_modules.end()) {
                AKLOG_ERRORN("Module not found");
                return render_prof;
            }

            std::string conf_path;
            {
                std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
                conf_path = m_state->m_conf_path.to_str();
            }

            // Timer timer;
            // timer.start();

            py::object elem;
            if (py::hasattr(it->second->mod, "__akashi_export_elem_fn")) {
                elem = it->second->mod.attr("__akashi_export_elem_fn")(conf_path);
            }

            // timer.stop();
            // AKLOG_DEBUG("render_prof:: get_kron, time: {} sec",
            // timer.current_time().to_decimal()); timer.start();

            try {
                m_gctx = global_eval(elem, m_state->m_prop.fps);
            } catch (const std::exception& e) {
                AKLOG_ERROR("{}", e.what());
                return render_prof;
            }

            // timer.stop();
            // AKLOG_DEBUG("render_prof:: global_eval, time: {} sec",
            //             timer.current_time().to_decimal());

            render_prof.uuid = m_gctx->uuid;
            render_prof.duration = m_gctx->duration;
            for (const auto& atom_proxy : m_gctx->atom_proxies) {
                render_prof.atom_profiles.push_back(atom_proxy.computed_profile());
            }

            return render_prof;
        }

        void PyEvalContext::reload(const std::vector<watch::WatchEvent>& events) {
            for (const auto& event : events) {
                switch (event.flag) {
                    case watch::WatchEventFlag::CREATED: {
                        auto module_path = Path(event.file_path);
                        auto it = this->m_modules.find(module_path.to_str());
                        // check duplication
                        if (it == this->m_modules.end()) {
                            this->load_module(module_path, this->config().include_dir);
                        }
                        break;
                    }
                    case watch::WatchEventFlag::REMOVED: {
                        auto module_path = Path(event.file_path);
                        auto it = this->m_modules.find(module_path.to_str());
                        if (it != this->m_modules.end()) {
                            this->m_modules.erase(it);
                        }
                        break;
                    }
                    default: {
                    }
                }
            }

            // reload all modules
            for (auto it = this->m_modules.begin(); it != this->m_modules.end(); it++) {
                it->second->mod.reload();
            }

            auto shader_mod = py::module_::import("akashi_core").attr("pysl").attr("shader");
            shader_mod.attr("_invalidate_compile_cache")();
            shader_mod.attr("_invalidate_artifact_cache")();
        }

        const state::EvalConfig& PyEvalContext::config(void) {
            std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
            return m_state->m_prop.eval_state.config;
        }

        bool PyEvalContext::load_module(const core::Path& module_path,
                                        const core::Path& include_dir) {
            auto mod_ptr = core::make_owned<PyBind11Module>();
            mod_ptr->mod = py::module_::import(
                module_path.to_relpath(include_dir).to_pymodule_name().to_str());
            m_modules.insert({module_path.to_abspath().to_cloned_str(), std::move(mod_ptr)});

            AKLOG_DEBUG("Loaded Python module: {}", module_path.to_str());

            return true;
        }

        bool PyEvalContext::register_deps_module(const core::Path& entry_path,
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
