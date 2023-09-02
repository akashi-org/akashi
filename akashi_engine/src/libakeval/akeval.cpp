#include "./akeval.h"
#include "./item.h"
#include "./context.h"

#include "./backend/python.h"

#include <libakcore/memory.h>
#include <libakcore/logger.h>
#include <libakcore/perf.h>

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

        AKEval::~AKEval() { this->exit(); }

        void AKEval::exit(void) {
            if (!m_exited) {
                W_ASSERT();
                m_eval_ctx->exit();
                m_exited = true;
            }
        }

        core::RenderProfile AKEval::render_prof(const std::string& module_path,
                                                const std::string& elem_name) {
            if (m_eval_ctx->loaded()) {
                ASSERT();
                try {
                    return m_eval_ctx->render_prof(module_path, elem_name);
                } catch (const std::exception& e) {
                    AKLOG_ERROR("{}", e.what());
                }
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
