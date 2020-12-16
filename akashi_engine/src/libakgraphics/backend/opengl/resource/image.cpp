#include "./image.h"

#include <libakcore/logger.h>
#include <libakcore/error.h>

#include <SDL_image.h>
#include <stdexcept>

using namespace akashi::core;

namespace akashi {
    namespace graphics {

        ImageLoader::ImageLoader(void) {
            auto flags = IMG_INIT_PNG;
            if ((IMG_Init(IMG_INIT_PNG) & flags) != flags) {
                AKLOG_ERROR("ImageLoader::ImageLoader: IMG_Init failed\n{}", IMG_GetError());
                throw std::runtime_error("Failed to init ImageLoader");
            }
        };

        ImageLoader::~ImageLoader(void) {
            // [TODO] usually, this will not be called. how should we treat this?
            IMG_Quit();
        };

        ak_error_t ImageLoader::getSurface(SDL_Surface*& surface, const char* image_path) {
            surface = IMG_Load(image_path);
            if (!surface) {
                AKLOG_ERROR("ImageLoader::getSurface: IMG_Load failed\n{}", IMG_GetError());
                return ErrorType::Error;
            }
            return ErrorType::OK;
        }

    }
}
