#pragma once

#include "./resource.h"

#include <libakcore/element.h>

#include <SDL.h>
#include <string>

typedef struct SDL_Surface SDL_Surface;

namespace akashi {
    namespace graphics {

        struct FontInfo {
            std::string text;
            core::TextAlign text_align;
            std::array<int32_t, 4> pad;
            int32_t line_span;
            std::string font_path;
            SDL_Color color;
            int size;
        };

        struct FontOutline {
            SDL_Color color = {0, 0, 0, 255};
            int size = 0;
        };

        struct FontShadow {
            SDL_Color color = {0, 0, 0, 255};
            int size = 0;
        };

        class FontLoader final : public ResourceLoader<FontLoader> {
          public:
            friend class ResourceLoader<FontLoader>;

          public:
            bool get_surface(SDL_Surface*& surface, const FontInfo& info,
                             const FontOutline* outline = nullptr,
                             const FontShadow* shadow = nullptr);

          private:
            explicit FontLoader(void);
            virtual ~FontLoader(void);

            bool get_surface_normal(SDL_Surface*& surface, const FontInfo& info,
                                    const FontOutline* outline = nullptr);

            bool get_surface_outline(SDL_Surface*& surface, const FontInfo& info,
                                     const FontOutline& outline);

            bool get_surface_shadow(SDL_Surface*& surface, const FontInfo& info,
                                    const FontShadow& outline);
        };

        SDL_Color hex_to_sdl(std::string input);

    }
}
