#include "./context.h"
#include "./parser.h"

#include "../../item.h"
#include "./utils/pytype.h"
#include "./core/string.h"
#include "./core/module.h"
#include "./core/func.h"
#include "./core/tuple.h"
#include "./core/list.h"
#include "./core/import.h"
#include "./libraries/akashi_core/init.h"
#include "./libraries/akashi_core/time.h"
#include "./libraries/akashi_core/kron.h"
#include "./libraries/akashi_core/kron_utils.h"

#include <libakstate/akstate.h>
#include <libakcore/path.h>
#include <libakcore/memory.h>
#include <libakcore/logger.h>
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

#include "./utils/print.h"

using namespace akashi::core;

namespace akashi {
    namespace eval {

        static void version_check() {
            auto module = from_import_module("akashi_core", {"utils"});
            auto func = module->get_module()->attr("utils")->attr("version_check");

            auto res = make_owned<PythonObject>(PyObject_CallObject(func->get_raw(), nullptr));

            auto status = PythonValue::from(PyTuple_GetItem(res->get_raw(), 0), false).to<bool>();
            if (!status) {
                auto msg = PythonValue::from(PyTuple_GetItem(res->get_raw(), 1), false)
                               .to<PythonString>()
                               .char_p();

                AKLOG_WARN("{}", msg);
            }
        }

        PyEvalContext::PyEvalContext(core::borrowed_ptr<state::AKState> state)
            : EvalContext(state), m_state(state) {
            // maybe we could use Py_InitializeFromConfig instead.
            // ref: https://docs.python.org/3/c-api/init_config.html#initialization-with-pyconfig
            Py_Initialize();

            auto config = this->config();

            // Load PYTHONPATH explicitly!
            auto sysPath = PySys_GetObject("path");

            auto dir = PyUnicode_DecodeFSDefault(config.include_dir.to_abspath().to_cloned_str());
            // auto dir = PyUnicode_DecodeFSDefault(m_entry_path.to_dirpath().to_cloned_str());
            PyList_Append(sysPath, dir);
            // Do not XDECREF syscore::Path
            Py_XDECREF(dir);

            if (std::getenv("AK_CORELIB_PATH")) {
                auto corelib_path = PyUnicode_DecodeFSDefault(
                    core::Path(std::getenv("AK_CORELIB_PATH")).to_abspath().to_cloned_str());
                PyList_Insert(sysPath, 0, corelib_path);
                Py_XDECREF(corelib_path);
            }

            m_corelib = lib::akashi_core::InitModule::from();

            version_check();

            this->load_module(config.entry_path, config.include_dir);

            this->imported_inner_module_each([this, config](const core::Path& module_path) {
                this->load_module(module_path, config.include_dir);
            });
        };

        PyEvalContext::~PyEvalContext(void) noexcept { this->exit(); };

        // [TODO] since this is called from outside the eval thread, maybe we should use GIL?
        void PyEvalContext::exit(void) {
            for (auto it = m_modules.begin(); it != m_modules.end(); it++) {
                delete it->second;
            }

            if (Py_FinalizeEx() < 0) {
                AKLOG_ERRORN("PythonVM::exit(): Py_FinalizeEx() failed");
            } else {
                AKLOG_INFON("PythonVM::exit(): Successfully exited");
            }
        };

        core::FrameContext PyEvalContext::eval_kron(const char* module_path, const KronArg& arg) {
            FrameContext frame_ctx;
            frame_ctx.pts = Fraction{-1, 1};

            auto it = m_modules.find(module_path);
            if (it == m_modules.end()) {
                AKLOG_ERRORN("PyEvalContext::eval_kron() Module not found");
                return frame_ctx;
            }

            auto pymod = it->second;
            auto kron_func_name = Path(module_path).to_stem();

            auto func = PythonFunc(pymod->get_module(), kron_func_name.to_str());

            auto comp_args = m_corelib->kron()->KronArgs(arg);

            auto kron = PythonObject(PyObject_CallObject(func.get_func()->get_raw(), nullptr));

            auto frame = make_owned<PythonObject>(PyObject_CallObject(
                kron.get_raw(), make_tuple(comp_args->inst_obj()->get_raw())->get_raw()));

            if (frame->get_raw() == nullptr) {
                if (PyErr_Occurred() != nullptr) {
                    PyErr_Print();
                }
                return frame_ctx;
            }
            frame_ctx = parse_frameContext(borrowed_ptr(frame));

            return frame_ctx;
        };

        std::vector<core::FrameContext> PyEvalContext::eval_krons(const char* module_path,
                                                                  const core::Rational& start_time,
                                                                  const int fps,
                                                                  const core::Rational& duration,
                                                                  const size_t length) {
            std::vector<FrameContext> frame_ctxs;

            auto it = m_modules.find(module_path);
            if (it == m_modules.end()) {
                AKLOG_ERRORN("PyEvalContext::eval_krons() Module not found");
                return frame_ctxs;
            }

            auto pymod = it->second;
            auto kron_func_name = Path(module_path).to_stem();

            auto elem = PythonFunc(pymod->get_module(), kron_func_name.to_str());

            // from here, do not refer kron
            auto frame_ctxs_obj = m_corelib->kron_utils()->get_frame_contexts(
                make_owned<PythonObject>(elem.get_func()->get_raw(), false), start_time, fps,
                duration, length);

            // py_print(frame_ctxs_obj->get_raw());

            frame_ctxs = map_list<FrameContext>(
                frame_ctxs_obj->get_raw(),
                [](borrowed_ptr<const PythonObject> pyo) { return parse_frameContext(pyo); });

            return frame_ctxs;
        }

        core::RenderProfile PyEvalContext::render_prof(const char* module_path) {
            RenderProfile render_prof;

            auto it = m_modules.find(module_path);
            if (it == m_modules.end()) {
                AKLOG_ERRORN("PyEvalContext::render_prof() Module not found");
                return render_prof;
            }

            auto pymod = it->second;
            auto kron_func_name = Path(module_path).to_stem();

            auto kron = PythonFunc(pymod->get_module(), kron_func_name.to_str());

            KronArg arg;
            arg.fps = 24; // [TODO] temporary hack

            // from here, do not refer kron
            auto render_prof_obj = m_corelib->kron_utils()->get_render_profile(
                make_owned<PythonObject>(kron.get_func()->get_raw(), false), arg);

            // py_print(render_prof_obj->get_raw());

            render_prof = parse_renderProfile(borrowed_ptr(render_prof_obj));

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
                            delete it->second;
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
                it->second->reload();
            }

            // [XXX] we can skip reloading for corelib
        }

        const std::vector<std::string>
        PyEvalContext::loaded_module_paths(bool kron_module_only) const {
            std::vector<std::string> loaded_module_paths;

            for (auto it = m_modules.begin(); it != m_modules.end(); it++) {
                // if (!kron_module_only || it->second->has_kron()) {
                //     loaded_module_paths.push_back(it->first);
                // }
                loaded_module_paths.push_back(it->first);
            }

            return loaded_module_paths;
        };

        const std::vector<std::string> PyEvalContext::imported_module_paths(void) {
            std::vector<std::string> imported_module_paths;

            this->imported_module_each([&imported_module_paths](PyObject* module) {
                if (PyObject_HasAttrString(module, "__file__")) {
                    auto module_fpath_obj = PyObject_GetAttrString(module, "__file__");
                    if (PyUnicode_Check(module_fpath_obj)) {
                        imported_module_paths.push_back(
                            core::Path(PythonString(module_fpath_obj).char_p()).to_cloned_str());
                    }
                    Py_XDECREF(module_fpath_obj);
                }
            });

            return imported_module_paths;
        };

        const state::EvalConfig& PyEvalContext::config(void) {
            std::lock_guard<std::mutex> lock(m_state->m_prop_mtx);
            return m_state->m_prop.eval_state.config;
        }

        bool PyEvalContext::load_module(const core::Path& module_path,
                                        const core::Path& include_dir) {
            m_modules.insert({module_path.to_abspath().to_cloned_str(),
                              new PythonModule(module_path.to_relpath(include_dir))});

            AKLOG_DEBUG("PythonVM::load_modules() loaded, {}", module_path.to_str());

            return true;
        };

        void PyEvalContext::imported_module_each(const std::function<void(PyObject*)>& callback) {
            auto module_dict = PyImport_GetModuleDict(); /* borrowed */
            // auto module_dict = PyModule_GetDict();

            auto modules = PyDict_Values(module_dict);
            for (int i = 0; i < PyList_GET_SIZE(modules); i++) {
                auto module = PyList_GetItem(modules, i); /* borrowed */
                if (!module) {
                    PyErr_Print();
                }
                callback(module);
            }
        };

        void PyEvalContext::imported_inner_module_each(
            const std::function<void(const core::Path&)>& callback) {
            this->imported_module_each([this, &callback](PyObject* module) {
                if (PyObject_HasAttrString(module, "__file__")) {
                    auto module_fpath_obj = PyObject_GetAttrString(module, "__file__");
                    if (PyUnicode_Check(module_fpath_obj)) {
                        auto module_fpath = core::Path(PythonString(module_fpath_obj).char_p());
                        auto config = this->config();
                        if (module_fpath != config.entry_path &&
                            module_fpath.is_subordinate(config.include_dir)) {
                            callback(module_fpath);
                        }
                    }
                    Py_XDECREF(module_fpath_obj);
                }
            });
        };

    }
}
