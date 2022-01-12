#include "./time.h"

#include "./rational.h"

#include <boost/date_time/posix_time/posix_time.hpp>

#include <sstream>

namespace btime = boost::posix_time;

namespace akashi {
    namespace core {

        static btime::time_facet* s_output_facet = nullptr;
        static std::locale* s_locale = nullptr;

        std::string to_time_string(long time_ms, bool format_msec) {
            if (!s_output_facet) {
                s_output_facet = new btime::time_facet();
                s_output_facet->time_duration_format("%H:%M:%S");
                s_locale = new std::locale(std::locale::classic(), s_output_facet);
            }

            long hour_head = 0;
            // a case with time_ms >= 100 hours
            if (time_ms >= 360000000) {
                hour_head = time_ms / 360000000;
                time_ms = time_ms - (360000000 * hour_head);
            }

            btime::time_duration td = btime::milliseconds(time_ms);

            std::ostringstream oss;
            if (hour_head > 0) {
                oss << hour_head;
            }
            oss.imbue(*s_locale);
            oss << td;
            if (format_msec) {
                oss << "." << std::setfill('0') << std::setw(3)
                    << (long)(td.fractional_seconds() / 1000);
            }
            return oss.str();
        }
    }
}
