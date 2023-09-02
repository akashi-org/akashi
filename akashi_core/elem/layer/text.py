# pyright: reportPrivateUsage=false
from __future__ import annotations
from dataclasses import dataclass, field
import typing as tp

from akashi_core.color import Color as ColorEnum
from akashi_core.color import color_value

from .base import (
    _BaseTrait,
    _BaseTraitField,
)


''' Text Concept '''

TextAlign = tp.Literal['left', 'center', 'right']


@dataclass
class TextField(_BaseTraitField):
    req_text: str = ''
    text_align: TextAlign = 'left'
    pad: tuple[int, int, int, int] = (0, 0, 0, 0)  # left, right, top, bottom
    line_span: int = 0


@dataclass
class TextTrait(_BaseTrait):
    _name: str = "text"

    def text_align(self, align: TextAlign) -> tp.Self:
        self._priv.get_trait_field(self).text_align = align
        return self

    def pad_x(self, a: int, b: tp.Optional[int] = None) -> tp.Self:
        trait_field = self._priv.get_trait_field(self)
        left_pad = a
        right_pad = a if not b else b
        rests = trait_field.pad[2:]
        trait_field.pad = (left_pad, right_pad, *rests)
        return self

    def pad_y(self, a: int, b: tp.Optional[int] = None) -> tp.Self:
        trait_field = self._priv.get_trait_field(self)
        top_pad = a
        bottom_pad = a if not b else b
        rests = trait_field.pad[:2]
        trait_field.pad = (*rests, top_pad, bottom_pad)
        return self

    def line_span(self, span: int) -> tp.Self:
        self._priv.get_trait_field(self).line_span = span
        return self


''' TextStyle Concept '''


@dataclass
class TextStyleField(_BaseTraitField):
    font_path: str = ""
    fg_size: int = 30
    fg_color: str = "#ffffff"  # "#rrggbb" or "#rrggbbaa"
    use_outline: bool = False
    outline_size: int = 0
    outline_color: str = "#000000"  # "#rrggbb" or "#rrggbbaa"
    use_shadow: bool = False
    shadow_size: int = 0
    shadow_color: str = "#000000"  # "#rrggbb" or "#rrggbbaa"


@dataclass
class TextStyleTrait(_BaseTrait):
    _name: str = "text_style"

    def font_path(self, path: str) -> tp.Self:
        self._priv.get_trait_field(self).font_path = path
        return self

    def fg(self, fg_color: tp.Union[str, ColorEnum], fg_size: int) -> tp.Self:
        self._priv.get_trait_field(self).fg_color = color_value(fg_color)
        self._priv.get_trait_field(self).fg_size = fg_size
        return self

    def outline(self, outline_color: tp.Union[str, ColorEnum], outline_size: int) -> tp.Self:
        self._priv.get_trait_field(self).use_shadow = False
        self._priv.get_trait_field(self).use_outline = True
        self._priv.get_trait_field(self).outline_color = color_value(outline_color)
        self._priv.get_trait_field(self).outline_size = outline_size
        return self

    def shadow(self, shadow_color: tp.Union[str, ColorEnum], shadow_size: int) -> tp.Self:
        self._priv.get_trait_field(self).use_outline = False
        self._priv.get_trait_field(self).use_shadow = True
        self._priv.get_trait_field(self).shadow_color = color_value(shadow_color)
        self._priv.get_trait_field(self).shadow_size = shadow_size
        return self
