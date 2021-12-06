from __future__ import annotations
from dataclasses import dataclass, field
import typing as tp
from abc import abstractmethod, ABCMeta

from akashi_core.elem.context import _GlobalKronContext as gctx
from akashi_core.time import sec
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


@dataclass
class TextStyle:
    font_size: int = 30
    font_path: str = ""
    fill: str = ""  # rrggbb


@dataclass
class TextLocalField:
    text: str
    style: TextStyle = field(init=False)


@dataclass
class TextEntry(ShaderField, PositionField, LayerField, TextLocalField):
    ...

    def __post_init__(self):
        self.style = TextStyle()


@dataclass
class TextHandle(FittableDurationTrait, ShaderTrait, PositionTrait, LayerTrait):

    def duration(self, duration: sec) -> 'TextHandle':
        return super().duration(duration)

    def pos(self, x: int, y: int) -> 'TextHandle':
        return super().pos(x, y)

    def z(self, value: float) -> 'TextHandle':
        return super().z(value)

    def frag(self, frag_shader: FragShader) -> 'TextHandle':
        return super().frag(frag_shader)

    def poly(self, poly_shader: PolygonShader) -> 'TextHandle':
        return super().poly(poly_shader)

    def fit_to(self, handle: 'AtomHandle') -> 'TextHandle':
        return super().fit_to(handle)

    def font_size(self, size: int) -> 'TextHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, TextEntry):
            cur_layer.style.font_size = size
        return self

    def font_path(self, path: str) -> 'TextHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, TextEntry):
            cur_layer.style.font_path = path
        return self

    def font_fill(self, color: str) -> 'TextHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, TextEntry):
            cur_layer.style.fill = color
        return self


def text(text: str, key: str = '') -> TextHandle:

    entry = TextEntry(text)
    idx = register_entry(entry, 'TEXT', key)
    return TextHandle(idx)
