#pragma once

#include "./resource.h"

#include <libakcore/error.h>

typedef struct SDL_Surface SDL_Surface;

namespace akashi {
    namespace graphics {

        class ImageLoader final : public ResourceLoader<ImageLoader> {
          public:
            friend class ResourceLoader<ImageLoader>;

          public:
            static ak_error_t getSurface(SDL_Surface*& surface, const char* image_path);

          private:
            explicit ImageLoader(void);
            virtual ~ImageLoader(void);
        };

    }
}
