#pragma once

#include <execinfo.h>
#include <stdlib.h>
#include <cxxabi.h>
#include <string>

namespace akashi {
    namespace core {

        namespace detail {

            inline std::string get_mangled_func(const std::string& trace_str) {
                auto begin_tk = trace_str.find("(_Z");
                auto end_tk = trace_str.find("+0x", begin_tk + 1);
                return (begin_tk == trace_str.npos || end_tk == trace_str.npos)
                           ? ""
                           : trace_str.substr(begin_tk + 1, end_tk - begin_tk - 1);
            }
        }

        inline std::string collect_stacktrace(const int max_stack_size) {
            std::string stacktrace_str;
            stacktrace_str += "\nStackTrace: \n";
            void* array[max_stack_size];
            size_t size = 0;
            char** trace_strs;
            size = backtrace(array, max_stack_size);
            trace_strs = backtrace_symbols(array, size);
            if (trace_strs) {
                for (size_t i = 0; i < size; i++) {
                    stacktrace_str += "  #" + std::to_string(i) + " ";
                    int status = -1;
                    auto demangled_trace = abi::__cxa_demangle(
                        detail::get_mangled_func(trace_strs[i]).c_str(), nullptr, 0, &status);
                    if (status == 0 && demangled_trace) {
                        stacktrace_str += demangled_trace;
                        free(demangled_trace);
                    } else {
                        stacktrace_str += trace_strs[i];
                    }
                    if (i != size - 1) {
                        stacktrace_str += "\n";
                    }
                }
            }
            free(trace_strs);

            return stacktrace_str;
        }

    }
}
