#include "./process.h"
#include "../kernel_event.h"
#include "../kernel_state.h"

#include <libakcore/memory.h>
#include <libakcore/logger.h>
#include <libakcore/config.h>

#include <boost/process.hpp>
#include <iostream>
using namespace boost::process;

using namespace akashi::core;

namespace akashi {
    namespace kernel {

        struct ProcessWorker::ExitContext {
            ProcessWorkerContext ctx;
            ProcessWorker* worker;
            boost::process::child* child_process = nullptr;
        };

        struct ProcessWorker::PrivProcess {
            boost::process::child* child = nullptr;
            boost::process::ipstream* ipst = nullptr;
            boost::process::opstream* opst = nullptr;
        };

        std::string ProcessWorker::pass_through(const std::string& req_str) {
            std::string res;
            {
                std::lock_guard<std::mutex> lock(m_priv_process.mtx);
                if (!m_priv_process.value) {
                    return res;
                }
                auto child = m_priv_process.value->child;
                if (child && child->valid() && child->running()) {
                    auto ipst = m_priv_process.value->ipst;
                    auto opst = m_priv_process.value->opst;
                    if (ipst && opst) {
                        *opst << req_str << std::endl;
                        *ipst >> res;
                    }
                }
            }
            return res;
        }

        void ProcessWorker::process_thread(ProcessWorkerContext ctx, ProcessWorker* worker) {
            AKLOG_INFON("Process init");

            ExitContext exit_ctx{ctx, worker};

            worker->set_on_thread_exit(
                [](void* ctx_) {
                    auto exit_ctx_ = reinterpret_cast<ExitContext*>(ctx_);
                    ProcessWorker::exit_thread(*exit_ctx_);
                    AKLOG_INFON("Process successfully exited");
                },
                &exit_ctx);

            boost::process::ipstream out;
            boost::process::opstream in;

            std::string cmd_str =
                ctx.state->prop().renderer_path + " " + "\"" + ctx.state->prop().config_jstr + "\"";
            auto c = new boost::process::child{cmd_str, std_out > out, std_in < in};
            exit_ctx.child_process = c;

            {
                std::lock_guard<std::mutex> lock(worker->m_priv_process.mtx);
                if (!worker->m_priv_process.value) {
                    worker->m_priv_process.value = new PrivProcess;
                }
                worker->m_priv_process.value->child = c;
                worker->m_priv_process.value->ipst = &out;
                worker->m_priv_process.value->opst = &in;
            }

            c->wait();

            KernelEventQueueData evt_data;
            evt_data.evt = KernelEvent::PROCESS_EXIT;
            KernelEventParams<KernelEvent::PROCESS_EXIT> params;
            params.exit_code = c->exit_code();
            evt_data.params = params;

            ctx.queue->enqueue(std::move(evt_data));

            ProcessWorker::exit_thread(exit_ctx);
            AKLOG_INFON("Process finished");
        };

        void ProcessWorker::exit_thread(ExitContext& exit_ctx) {
            if (exit_ctx.worker) {
                exit_ctx.worker->m_thread_alive = false;
                {
                    std::lock_guard<std::mutex> lock(exit_ctx.worker->m_priv_process.mtx);
                    delete exit_ctx.worker->m_priv_process.value;
                    exit_ctx.worker->m_priv_process.value = nullptr;
                }
            }
            if (exit_ctx.child_process) {
                exit_ctx.child_process->terminate();
                delete exit_ctx.child_process;
                exit_ctx.child_process = nullptr;
            }
        }

    }
}
