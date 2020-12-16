#include "./font.h"

#include <libakcore/logger.h>
#include <libakcore/error.h>

#include <SDL_ttf.h>
#include <stdexcept>
#include <memory>

using namespace akashi::core;

namespace akashi {
    namespace graphics {

        FontLoader::FontLoader(void) {
            if (TTF_Init() == -1) {
                AKLOG_ERROR("FontLoader::FontLoader: TTF_Init failed\n{}", TTF_GetError());
                throw std::runtime_error("Failed to init FontLoader");
            }
        };

        FontLoader::~FontLoader(void) {
            // [TODO] usually, this will not be called. how should we treat this?
            TTF_Quit();
        };

        ak_error_t FontLoader::getSurface(SDL_Surface*& surface, const FontInfo& info) {
            auto font = TTF_OpenFont(info.font_path, info.font_size);
            if (!font) {
                AKLOG_ERROR("FontLoader::getSurface: TTF_OpenFont failed\n{}", TTF_GetError());
                return ErrorType::Error;
            }
            // TTF_SetFontOutline(font, 1);
            surface = TTF_RenderUTF8_Blended(font, info.text, info.fg);
            if (!surface) {
                TTF_CloseFont(font);
                AKLOG_ERROR("FontLoader::getFontSurface: TTF_RenderUTF8_Blended failed\n{}",
                            TTF_GetError());
                return ErrorType::Error;
            }
            TTF_CloseFont(font);
            return ErrorType::OK;
        }

        SDL_Color hex_to_sdl(std::string input) {
            SDL_Color color = {0, 0, 0, 255};

            if (input[0] == '#') {
                input.erase(0, 1);
            } else {
                return color;
            }

            unsigned long value = stoul(input, nullptr, 16);

            // color.a = (value >> 24) & 0xff;
            color.r = (value >> 16) & 0xff;
            color.g = (value >> 8) & 0xff;
            color.b = (value >> 0) & 0xff;
            return color;
        }

    }
}
