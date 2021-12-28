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

        AKEval::AKEval(core::borrowed_ptr<state::AKState> state) : m_state(state) {
            m_eval_ctx = make_owned<PyEvalContext>(m_state);
            m_tid = std::this_thread::get_id();

            try {
                m_eval_ctx->load();
            } catch (const std::exception& e) {
                AKLOG_ERROR("{}", e.what());
            }
        }

        AKEval::~AKEval() {}

        void AKEval::exit(void) {
            W_ASSERT();
            m_eval_ctx->exit();
        }

        core::FrameContext AKEval::eval_kron(const char* module_path, const KronArg& kron_arg) {
            if (m_eval_ctx->loaded()) {
                ASSERT();
                return m_eval_ctx->eval_kron(module_path, kron_arg);
            }

            FrameContext frame_ctx;
            frame_ctx.pts = Rational{-1, 1};
            return frame_ctx;
        }

        std::vector<core::FrameContext>
        AKEval::eval_krons(const char* module_path, const core::Rational& start_time, const int fps,
                           const core::Rational& duration, const size_t length) {
            if (m_eval_ctx->loaded()) {
                ASSERT();
                return m_eval_ctx->eval_krons(module_path, start_time, fps, duration, length);
            }

            return {};
        }

        core::RenderProfile AKEval::render_prof(const std::string& module_path,
                                                const std::string& elem_name) {
            if (m_eval_ctx->loaded()) {
                ASSERT();
                return m_eval_ctx->render_prof(module_path, elem_name);
            }

            RenderProfile render_prof;
            render_prof.atom_profiles = {};

            return render_prof;
        }

        void AKEval::reload(const std::vector<watch::WatchEvent>& events) {
            W_ASSERT();
            try {
                if (!m_eval_ctx->loaded()) {
                    m_eval_ctx->load();
                } else {
                    m_eval_ctx->reload(events);
                }
            } catch (const std::exception& e) {
                AKLOG_ERROR("{}", e.what());
            }
        }

    }
}
