#include "./font.h"

#include <libakcore/logger.h>
#include <libakcore/error.h>
#include <libakcore/string.h>

#include <SDL_ttf.h>
#include <stdexcept>
#include <memory>

using namespace akashi::core;

namespace akashi {
    namespace graphics {

        FontLoader::FontLoader(void) {
            if (TTF_Init() == -1) {
                AKLOG_ERROR("TTF_Init failed\n{}", TTF_GetError());
                throw std::runtime_error("Failed to init FontLoader");
            }
        };

        FontLoader::~FontLoader(void) {
            // [TODO] usually, this will not be called. how should we treat this?
            TTF_Quit();
        };

        bool FontLoader::get_surface(SDL_Surface*& surface, const FontInfo& info,
                                     const FontOutline* outline, const FontShadow* shadow) {
            auto lines = core::split_by(info.text, "\n");
            std::vector<SDL_Surface*> line_surfaces(lines.size(), nullptr);

            int main_width = 0;
            int main_height = 0;
            for (size_t i = 0; i < lines.size(); i++) {
                FontInfo line_info = info;
                line_info.text = lines[i];

                bool result = false;
                if (shadow) {
                    result = this->get_surface_shadow(line_surfaces[i], line_info, *shadow);
                }
                if (outline) {
                    result = this->get_surface_outline(line_surfaces[i], line_info, *outline);
                }
                result = this->get_surface_normal(line_surfaces[i], line_info);

                if (!result) {
                    for (auto&& ls : line_surfaces) {
                        SDL_FreeSurface(ls);
                    }
                    return false;
                }

                main_width = std::max(main_width, line_surfaces[i]->w);
                main_height += line_surfaces[i]->h;
            }

            SDL_Surface* main_surface =
                SDL_CreateRGBSurface(0, main_width + info.pad[0] + info.pad[1],
                                     main_height + info.pad[2] + info.pad[3], 32, 0, 0, 0, 0);
            int err_code = 0;
            int acc_height = 0;
            for (auto&& ln_surface : line_surfaces) {
                int lw = info.text_align == TextAlign::RIGHT    ? main_width - ln_surface->w
                         : info.text_align == TextAlign::CENTER ? (main_width - ln_surface->w) / 2
                                                                : 0;
                SDL_Rect rect = {lw + (int)info.pad[0], acc_height + (int)info.pad[2],
                                 main_surface->w, main_surface->h};

                err_code += SDL_BlitSurface(ln_surface, nullptr, main_surface, &rect);
                acc_height += ln_surface->h;
                SDL_FreeSurface(ln_surface);
            }

            if (err_code != 0) {
                AKLOG_ERROR("{}", SDL_GetError());
                SDL_FreeSurface(main_surface);
                return false;
            }

            surface = main_surface;
            return true;
        }

        bool FontLoader::get_surface_normal(SDL_Surface*& surface, const FontInfo& info,
                                            const FontOutline* outline) {
            auto font = TTF_OpenFont(info.font_path.c_str(), info.size);
            if (!font) {
                AKLOG_ERROR("TTF_OpenFont failed\n{}", TTF_GetError());
                return false;
            }
            if (outline && outline->size > 0) {
                TTF_SetFontOutline(font, outline->size);
            }
            surface = TTF_RenderUTF8_Blended(font, info.text.c_str(),
                                             outline ? outline->color : info.color);
            TTF_CloseFont(font);
            if (!surface) {
                AKLOG_ERROR("TTF_RenderUTF8_Blended failed\n{}", TTF_GetError());
                return false;
            }
            return true;
        }

        bool FontLoader::get_surface_outline(SDL_Surface*& surface, const FontInfo& info,
                                             const FontOutline& outline) {
            SDL_Surface* fg_surface = nullptr;
            if (!this->get_surface_normal(fg_surface, info)) {
                return false;
            }

            SDL_Surface* outline_surface = nullptr;
            if (!this->get_surface_normal(outline_surface, info, &outline)) {
                return false;
            }

            int err_code = 0;
            SDL_Rect rect = {outline.size, outline.size, fg_surface->w, fg_surface->h};
            err_code += SDL_SetSurfaceBlendMode(fg_surface, SDL_BLENDMODE_BLEND);
            err_code += SDL_BlitSurface(fg_surface, nullptr, outline_surface, &rect);
            SDL_FreeSurface(fg_surface);

            if (err_code != 0) {
                AKLOG_ERROR("{}", SDL_GetError());
                SDL_FreeSurface(fg_surface);
                SDL_FreeSurface(outline_surface);
                return false;
            }

            surface = outline_surface;
            return true;
        }

        bool FontLoader::get_surface_shadow(SDL_Surface*& surface, const FontInfo& info,
                                            const FontShadow& shadow) {
            SDL_Surface* fg_surface = nullptr;
            if (!this->get_surface_normal(fg_surface, info)) {
                return false;
            }

            FontOutline outline;
            outline.color = shadow.color;
            outline.size = shadow.size;
            SDL_Surface* shadow_surface = nullptr;
            if (!this->get_surface_normal(shadow_surface, info, &outline)) {
                return false;
            }

            int err_code = 0;
            SDL_Rect rect = {0, 0, fg_surface->w, fg_surface->h};
            err_code += SDL_SetSurfaceBlendMode(fg_surface, SDL_BLENDMODE_BLEND);
            err_code += SDL_BlitSurface(fg_surface, nullptr, shadow_surface, &rect);
            SDL_FreeSurface(fg_surface);

            if (err_code != 0) {
                AKLOG_ERROR("{}", SDL_GetError());
                SDL_FreeSurface(fg_surface);
                SDL_FreeSurface(shadow_surface);
                return false;
            }

            surface = shadow_surface;
            return true;
        }

        SDL_Color hex_to_sdl(std::string input) {
            SDL_Color color = {0, 0, 0, 255};

            if (input[0] == '#') {
                input.erase(0, 1);
            } else {
                return color;
            }

            unsigned long value = stoul(input, nullptr, 16);

            int offset = 0;
            if (input.size() > 6) {
                color.a = (value >> 0) & 0xff;
                offset = 8;
            }
            color.r = (value >> (0 + offset)) & 0xff;
            color.g = (value >> (8 + offset)) & 0xff;
            color.b = (value >> (16 + offset)) & 0xff;
            return color;
        }

    }
}
