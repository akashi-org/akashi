#include "./akeval.h"
#include "./item.h"
#include "./context.h"

#include "./backend/python.h"

#include <libakcore/memory.h>
#include <libakcore/logger.h>

#include <thread>
#include <cassert>

using namespace akashi::core;

#define ASSERT() assert(m_tid == std::this_thread::get_id())

#define W_ASSERT()                                                                                 \
    if (m_tid != std::this_thread::get_id()) {                                                     \
        AKLOG_INFON("called from outside the eval thread");                                        \
    }

namespace akashi {
    namespace eval {

        AKEval::AKEval(core::borrowed_ptr<state::AKState> state) {
            m_eval_ctx = make_owned<PyEvalContext>(state);
            m_tid = std::this_thread::get_id();
        }

        AKEval::~AKEval() {}

        void AKEval::exit(void) {
            W_ASSERT();
            m_eval_ctx->exit();
        }

        core::FrameContext AKEval::eval_kron(const char* module_path, const KronArg& kron_arg) {
            ASSERT();
            return m_eval_ctx->eval_kron(module_path, kron_arg);
        }

        std::vector<core::FrameContext>
        AKEval::eval_krons(const char* module_path, const core::Rational& start_time, const int fps,
                           const core::Rational& duration, const size_t length) {
            ASSERT();
            return m_eval_ctx->eval_krons(module_path, start_time, fps, duration, length);
        }

        core::RenderProfile AKEval::render_prof(const char* module_path) {
            ASSERT();
            return m_eval_ctx->render_prof(module_path);
        }

        void AKEval::reload(const std::vector<watch::WatchEvent>& events) {
            ASSERT();
            return m_eval_ctx->reload(events);
        }

    }
}
