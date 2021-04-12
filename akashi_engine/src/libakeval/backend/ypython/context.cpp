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

        YPyEvalContext::YPyEvalContext(core::borrowed_ptr<state::AKState> state)
            : EvalContext(state), m_state(state) {
            AKLOG_DEBUGN("YPyEvalContext init");

            py::scoped_interpreter guard{}; // start the interpreter and keep it alive

            py::print("Hello, World!"); // use the Python API
        };

        YPyEvalContext::~YPyEvalContext(void) noexcept { this->exit(); };

        // [TODO] since this is called from outside the eval thread, maybe we should use GIL?
        void YPyEvalContext::exit(void){};

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

    }
}
