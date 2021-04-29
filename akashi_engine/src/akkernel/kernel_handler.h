#pragma once

#include <libakcore/memory.h>

namespace akashi {
    namespace kernel {

        class TransportWorker;
        class ProcessWorker;
        struct KernelState;
        class KernelEventQueue;
        struct KernelEventQueueData;

        struct HandlerContext {
            core::borrowed_ptr<TransportWorker> transport_worker;
            core::borrowed_ptr<ProcessWorker> process_worker;
            core::borrowed_ptr<KernelState> state;
            core::borrowed_ptr<KernelEventQueue> event_queue;
            core::borrowed_ptr<KernelEventQueueData> event_data;
        };

        void handle_asp_request_received(HandlerContext& ctx);

        void handle_process_exit(HandlerContext& ctx);

    }
}
