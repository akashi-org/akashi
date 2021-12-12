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
class RectDetail:
    width: int = 0
    height: int = 0


@dataclass
class CircleDetail:
    circle_radius: float = 0


''' Shape Concept '''

ShapeKind = tp.Literal['RECT', 'CIRCLE', 'ELLIPSE', 'LINE']


@dataclass
class ShapeField:
    shape_kind: ShapeKind
    fill: bool = True
    color: str = ""  # "#rrggbb" or "#rrggbbaa"
    border_size: float = 0
    edge_radius: float = 0
    rect: RectDetail = field(init=False)


class ShapeTrait(LayerTrait, metaclass=ABCMeta):

    def fill(self, enable_fill: bool):
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(ShapeField, cur_layer).fill = enable_fill
        return self

    def color(self, color: str):
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(ShapeField, cur_layer).color = color
        return self

    def border_size(self, size: float):
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(ShapeField, cur_layer).border_size = size
        return self

    def edge_radius(self, radius: float):
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(ShapeField, cur_layer).edge_radius = radius
        return self


@dataclass
class ShapeEntry(LayerField, ShaderField, PositionField, ShapeField):

    def __post_init__(self):
        self.rect = RectDetail()


@dataclass
class RectHandle(FittableDurationTrait, ShaderTrait, PositionTrait, ShapeTrait, LayerTrait):

    def fill(self, enable_fill: bool) -> 'RectHandle':
        return super().fill(enable_fill)

    def color(self, color: str) -> 'RectHandle':
        return super().color(color)

    def border_size(self, size: float) -> 'RectHandle':
        return super().border_size(size)

    def edge_radius(self, radius: float) -> 'RectHandle':
        return super().edge_radius(radius)

    def duration(self, duration: sec) -> 'RectHandle':
        return super().duration(duration)

    def pos(self, x: int, y: int) -> 'RectHandle':
        return super().pos(x, y)

    def z(self, value: float) -> 'RectHandle':
        return super().z(value)

    def frag(self, frag_shader: FragShader) -> 'RectHandle':
        return super().frag(frag_shader)

    def poly(self, poly_shader: PolygonShader) -> 'RectHandle':
        return super().poly(poly_shader)

    def fit_to(self, handle: 'AtomHandle') -> 'RectHandle':
        return super().fit_to(handle)


def rect(width: int, height: int, key: str = '') -> RectHandle:

    entry = ShapeEntry('RECT')
    entry.rect.width = width
    entry.rect.height = height
    idx = register_entry(entry, 'SHAPE', key)
    return RectHandle(idx)
