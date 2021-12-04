#pragma once

#include "./resource.h"

#include <libakcore/error.h>

#include <SDL.h>
#include <string>

typedef struct SDL_Color SDL_Color;
typedef struct SDL_Surface SDL_Surface;

namespace akashi {
    namespace graphics {

        struct FontInfo {
            std::string text;
            SDL_Color fg;
            std::string font_path;
            int font_size;
        };

        class FontLoader final : public ResourceLoader<FontLoader> {
          public:
            friend class ResourceLoader<FontLoader>;

          public:
            static ak_error_t getSurface(SDL_Surface*& surface, const FontInfo& info);

          private:
            explicit FontLoader(void);
            virtual ~FontLoader(void);
        };

        SDL_Color hex_to_sdl(std::string input);

    }
}
