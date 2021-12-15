from __future__ import annotations
from dataclasses import dataclass, field
import typing as tp
from abc import abstractmethod, ABCMeta

from akashi_core.elem.context import _GlobalKronContext as gctx
from akashi_core.time import sec
from akashi_core.color import Color as ColorEnum
from akashi_core.color import color_value
from akashi_core.pysl import FragShader, PolygonShader

from .base import (
    FittableDurationTrait,
    PositionField,
    PositionTrait,
    ShaderField,
    ShaderTrait,
    LayerField,
    LayerTrait
)
from .base import peek_entry, register_entry

if tp.TYPE_CHECKING:
    from akashi_core.elem.atom import AtomHandle


TextAlign = tp.Literal['left', 'center', 'right']


@dataclass
class TextStyle:
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
class TextLabel:
    color: str = "#ffffff00"  # "#rrggbb" or "#rrggbbaa"
    src: str = ""  # expects image path
    radius: float = 0
    frag_shader: tp.Optional[FragShader] = None
    poly_shader: tp.Optional[PolygonShader] = None


@dataclass
class TextBorder:
    color: str = "#ffffff00"  # "#rrggbb" or "#rrggbbaa"
    size: int = 0  # [TODO] float?
    radius: float = 0


@dataclass
class TextLocalField:
    text: str
    style: TextStyle = field(init=False)
    label: TextLabel = field(init=False)
    border: TextBorder = field(init=False)
    text_align: TextAlign = 'left'
    pad: tuple[int, int, int, int] = (0, 0, 0, 0)  # left, right, top, bottom
    line_span: int = 0


@dataclass
class TextEntry(ShaderField, PositionField, LayerField, TextLocalField):

    def __post_init__(self):
        self.style = TextStyle()
        self.label = TextLabel()
        self.border = TextBorder()


@dataclass
class TextHandle(FittableDurationTrait, ShaderTrait, PositionTrait, LayerTrait):

    def text_align(self, align: TextAlign) -> 'TextHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, TextEntry):
            cur_layer.text_align = align
        return self

    def pad_x(self, a: int, b: tp.Optional[int] = None) -> 'TextHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, TextEntry):
            left_pad = a
            right_pad = a if not b else b
            cur_layer.pad = (left_pad, right_pad, *cur_layer.pad[2:])
        return self

    def pad_y(self, a: int, b: tp.Optional[int] = None) -> 'TextHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, TextEntry):
            top_pad = a
            bottom_pad = a if not b else b
            cur_layer.pad = (*cur_layer.pad[:2], top_pad, bottom_pad)
        return self

    def line_span(self, span: int) -> 'TextHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, TextEntry):
            cur_layer.line_span = span
        return self

    def font_path(self, path: str) -> 'TextHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, TextEntry):
            cur_layer.style.font_path = path
        return self

    def fg(self, fg_color: tp.Union[str, ColorEnum], fg_size: int) -> 'TextHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, TextEntry):
            cur_layer.style.fg_color = color_value(fg_color)
            cur_layer.style.fg_size = fg_size
        return self

    def outline(self, outline_color: tp.Union[str, ColorEnum], outline_size: int) -> 'TextHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, TextEntry):
            cur_layer.style.use_shadow = False
            cur_layer.style.use_outline = True
            cur_layer.style.outline_color = color_value(outline_color)
            cur_layer.style.outline_size = outline_size
        return self

    def shadow(self, shadow_color: tp.Union[str, ColorEnum], shadow_size: int) -> 'TextHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, TextEntry):
            cur_layer.style.use_outline = False
            cur_layer.style.use_shadow = True
            cur_layer.style.shadow_color = color_value(shadow_color)
            cur_layer.style.shadow_size = shadow_size
        return self

    def border(self, color: tp.Union[str, ColorEnum], size: int, radius: float = 0) -> 'TextHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, TextEntry):
            cur_layer.border.color = color_value(color)
            cur_layer.border.size = size
            cur_layer.border.radius = radius
        return self

    def label_color(self, color: tp.Union[str, ColorEnum]) -> 'TextHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, TextEntry):
            cur_layer.label.color = color_value(color)
        return self

    def label_src(self, src: str) -> 'TextHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, TextEntry):
            cur_layer.label.src = src
        return self

    def label_radius(self, radius: float) -> 'TextHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, TextEntry):
            cur_layer.label.radius = radius
        return self

    def label_poly(self, poly: PolygonShader) -> 'TextHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, TextEntry):
            cur_layer.label.poly_shader = poly
        return self

    def label_frag(self, frag: FragShader) -> 'TextHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, TextEntry):
            cur_layer.label.frag_shader = frag
        return self


def text(text: str, key: str = '') -> TextHandle:

    entry = TextEntry(text)
    idx = register_entry(entry, 'TEXT', key)
    return TextHandle(idx)
