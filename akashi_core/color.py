from __future__ import annotations
import typing as tp
from enum import Enum
import colorsys

# color = tp.NewType('color', str)
_color_type = str


def to_hex(n: int) -> str:
    return format(n, '02x')


def rgb(r: int, g: int, b: int) -> _color_type:
    return rgba(r, g, b, 255)


def rgba(r: int, g: int, b: int, a: int) -> _color_type:
    return _color_type('#' + to_hex(r) + to_hex(g) + to_hex(b) + to_hex(a))


def hsv(h: int, s: int, v: int) -> _color_type:
    ''' h <- [0, 360], s <- [0, 100], v <- [0, 100] '''
    r, g, b = colorsys.hsv_to_rgb(h / 360, s / 100, v / 100)
    return rgb(int(r * 255), int(g * 255), int(b * 255))


class Color(Enum):
    # ref: https://developer.mozilla.org/en-US/docs/Web/CSS/color_value#color_keywords
    Black = "#000000"
    Silver = "#c0c0c0"
    Gray = "#808080"
    White = "#ffffff"
    Maroon = "#800000"
    Red = "#ff0000"
    Purple = "#800080"
    Fuchsia = "#ff00ff"
    Green = "#008000"
    Lime = "#00ff00"
    Olive = "#808000"
    Yellow = "#ffff00"
    Navy = "#000080"
    Blue = "#0000ff"
    Teal = "#008080"
    Aqua = "#00ffff"
    Orange = "#ffa500"
    Aliceblue = "#f0f8ff"
    Antiquewhite = "#faebd7"
    Aquamarine = "#7fffd4"
    Azure = "#f0ffff"
    Beige = "#f5f5dc"
    Bisque = "#ffe4c4"
    Blanchedalmond = "#ffebcd"
    Blueviolet = "#8a2be2"
    Brown = "#a52a2a"
    Burlywood = "#deb887"
    Cadetblue = "#5f9ea0"
    Chartreuse = "#7fff00"
    Chocolate = "#d2691e"
    Coral = "#ff7f50"
    Cornflowerblue = "#6495ed"
    Cornsilk = "#fff8dc"
    Crimson = "#dc143c"
    Darkblue = "#00008b"
    Darkcyan = "#008b8b"
    Darkgoldenrod = "#b8860b"
    Darkgray = "#a9a9a9"
    Darkgreen = "#006400"
    Darkgrey = "#a9a9a9"
    Darkkhaki = "#bdb76b"
    Darkmagenta = "#8b008b"
    Darkolivegreen = "#556b2f"
    Darkorange = "#ff8c00"
    Darkorchid = "#9932cc"
    Darkred = "#8b0000"
    Darksalmon = "#e9967a"
    Darkseagreen = "#8fbc8f"
    Darkslateblue = "#483d8b"
    Darkslategray = "#2f4f4f"
    Darkslategrey = "#2f4f4f"
    Darkturquoise = "#00ced1"
    Darkviolet = "#9400d3"
    Deeppink = "#ff1493"
    Deepskyblue = "#00bfff"
    Dimgray = "#696969"
    Dimgrey = "#696969"
    Dodgerblue = "#1e90ff"
    Firebrick = "#b22222"
    Floralwhite = "#fffaf0"
    Forestgreen = "#228b22"
    Gainsboro = "#dcdcdc"
    Ghostwhite = "#f8f8ff"
    Gold = "#ffd700"
    Goldenrod = "#daa520"
    Greenyellow = "#adff2f"
    Grey = "#808080"
    Honeydew = "#f0fff0"
    Hotpink = "#ff69b4"
    Indianred = "#cd5c5c"
    Indigo = "#4b0082"
    Ivory = "#fffff0"
    Khaki = "#f0e68c"
    Lavender = "#e6e6fa"
    Lavenderblush = "#fff0f5"
    Lawngreen = "#7cfc00"
    Lemonchiffon = "#fffacd"
    Lightblue = "#add8e6"
    Lightcoral = "#f08080"
    Lightcyan = "#e0ffff"
    Lightgoldenrodyellow = "#fafad2"
    Lightgray = "#d3d3d3"
    Lightgreen = "#90ee90"
    Lightgrey = "#d3d3d3"
    Lightpink = "#ffb6c1"
    Lightsalmon = "#ffa07a"
    Lightseagreen = "#20b2aa"
    Lightskyblue = "#87cefa"
    Lightslategray = "#778899"
    Lightslategrey = "#778899"
    Lightsteelblue = "#b0c4de"
    Lightyellow = "#ffffe0"
    Limegreen = "#32cd32"
    Linen = "#faf0e6"
    Mediumaquamarine = "#66cdaa"
    Mediumblue = "#0000cd"
    Mediumorchid = "#ba55d3"
    Mediumpurple = "#9370db"
    Mediumseagreen = "#3cb371"
    Mediumslateblue = "#7b68ee"
    Mediumspringgreen = "#00fa9a"
    Mediumturquoise = "#48d1cc"
    Mediumvioletred = "#c71585"
    Midnightblue = "#191970"
    Mintcream = "#f5fffa"
    Mistyrose = "#ffe4e1"
    Moccasin = "#ffe4b5"
    Navajowhite = "#ffdead"
    Oldlace = "#fdf5e6"
    Olivedrab = "#6b8e23"
    Orangered = "#ff4500"
    Orchid = "#da70d6"
    Palegoldenrod = "#eee8aa"
    Palegreen = "#98fb98"
    Paleturquoise = "#afeeee"
    Palevioletred = "#db7093"
    Papayawhip = "#ffefd5"
    Peachpuff = "#ffdab9"
    Peru = "#cd853f"
    Pink = "#ffc0cb"
    Plum = "#dda0dd"
    Powderblue = "#b0e0e6"
    Rosybrown = "#bc8f8f"
    Royalblue = "#4169e1"
    Saddlebrown = "#8b4513"
    Salmon = "#fa8072"
    Sandybrown = "#f4a460"
    Seagreen = "#2e8b57"
    Seashell = "#fff5ee"
    Sienna = "#a0522d"
    Skyblue = "#87ceeb"
    Slateblue = "#6a5acd"
    Slategray = "#708090"
    Slategrey = "#708090"
    Snow = "#fffafa"
    Springgreen = "#00ff7f"
    Steelblue = "#4682b4"
    Tan = "#d2b48c"
    Thistle = "#d8bfd8"
    Tomato = "#ff6347"
    Turquoise = "#40e0d0"
    Violet = "#ee82ee"
    Wheat = "#f5deb3"
    Whitesmoke = "#f5f5f5"
    Yellowgreen = "#9acd32"
    Rebeccapurple = "#663399"


def color_value(_color: tp.Union[str, Color]) -> str:

    if isinstance(_color, Color):
        return _color.value
    else:
        return _color
