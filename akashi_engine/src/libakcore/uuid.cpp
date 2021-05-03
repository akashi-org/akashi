#include "./uuid.h"

#include <boost/lexical_cast.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace akashi {
    namespace core {

        std::string uuid(void) {
            boost::uuids::random_generator gen;
            return boost::lexical_cast<std::string>(gen());
        }
    }
}
