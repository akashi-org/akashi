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
    lod: int = 64  # level of detail


@dataclass
class TriangleDetail:
    side: float = 0


LineStyle = tp.Literal['default', 'round-dot', 'square-dot', 'cap']


@dataclass
class LineDetail:
    size: float = 0
    begin: tuple[int, int] = (0, 0)
    end: tuple[int, int] = (0, 0)
    style: LineStyle = 'default'


''' Shape Concept '''

ShapeKind = tp.Literal['RECT', 'CIRCLE', 'ELLIPSE', 'TRIANGLE', 'LINE']


@dataclass
class ShapeField:
    shape_kind: ShapeKind
    fill: bool = True
    color: str = ""  # "#rrggbb" or "#rrggbbaa"
    border_size: float = 0
    edge_radius: float = 0
    rect: RectDetail = field(init=False)
    circle: CircleDetail = field(init=False)
    tri: TriangleDetail = field(init=False)
    line: LineDetail = field(init=False)


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
        self.circle = CircleDetail()
        self.tri = TriangleDetail()
        self.line = LineDetail()


@dataclass
class RectHandle(FittableDurationTrait, ShaderTrait, PositionTrait, ShapeTrait, LayerTrait):

    def ap(self, *hs: tp.Callable[['RectHandle'], 'RectHandle']) -> 'RectHandle':
        [h(self) for h in hs]
        return self

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


@dataclass
class CircleHandle(FittableDurationTrait, ShaderTrait, PositionTrait, ShapeTrait, LayerTrait):

    def ap(self, *hs: tp.Callable[['CircleHandle'], 'CircleHandle']) -> 'CircleHandle':
        [h(self) for h in hs]
        return self

    def fill(self, enable_fill: bool) -> 'CircleHandle':
        return super().fill(enable_fill)

    def color(self, color: str) -> 'CircleHandle':
        return super().color(color)

    def border_size(self, size: float) -> 'CircleHandle':
        return super().border_size(size)

    def duration(self, duration: sec) -> 'CircleHandle':
        return super().duration(duration)

    def pos(self, x: int, y: int) -> 'CircleHandle':
        return super().pos(x, y)

    def z(self, value: float) -> 'CircleHandle':
        return super().z(value)

    def frag(self, frag_shader: FragShader) -> 'CircleHandle':
        return super().frag(frag_shader)

    def poly(self, poly_shader: PolygonShader) -> 'CircleHandle':
        return super().poly(poly_shader)

    def fit_to(self, handle: 'AtomHandle') -> 'CircleHandle':
        return super().fit_to(handle)

    def lod(self, value: int) -> 'CircleHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, ShapeEntry):
            cur_layer.circle.lod = value
        return self


@dataclass
class TriangleHandle(FittableDurationTrait, ShaderTrait, PositionTrait, ShapeTrait, LayerTrait):

    def ap(self, *hs: tp.Callable[['TriangleHandle'], 'TriangleHandle']) -> 'TriangleHandle':
        [h(self) for h in hs]
        return self

    def fill(self, enable_fill: bool) -> 'TriangleHandle':
        return super().fill(enable_fill)

    def color(self, color: str) -> 'TriangleHandle':
        return super().color(color)

    def border_size(self, size: float) -> 'TriangleHandle':
        return super().border_size(size)

    def duration(self, duration: sec) -> 'TriangleHandle':
        return super().duration(duration)

    def pos(self, x: int, y: int) -> 'TriangleHandle':
        return super().pos(x, y)

    def z(self, value: float) -> 'TriangleHandle':
        return super().z(value)

    def frag(self, frag_shader: FragShader) -> 'TriangleHandle':
        return super().frag(frag_shader)

    def poly(self, poly_shader: PolygonShader) -> 'TriangleHandle':
        return super().poly(poly_shader)

    def fit_to(self, handle: 'AtomHandle') -> 'TriangleHandle':
        return super().fit_to(handle)


@dataclass
class LineHandle(FittableDurationTrait, ShaderTrait, PositionTrait, ShapeTrait, LayerTrait):

    def ap(self, *hs: tp.Callable[['LineHandle'], 'LineHandle']) -> 'LineHandle':
        [h(self) for h in hs]
        return self

    def color(self, color: str) -> 'LineHandle':
        return super().color(color)

    def duration(self, duration: sec) -> 'LineHandle':
        return super().duration(duration)

    def z(self, value: float) -> 'LineHandle':
        return super().z(value)

    def frag(self, frag_shader: FragShader) -> 'LineHandle':
        return super().frag(frag_shader)

    def poly(self, poly_shader: PolygonShader) -> 'LineHandle':
        return super().poly(poly_shader)

    def fit_to(self, handle: 'AtomHandle') -> 'LineHandle':
        return super().fit_to(handle)

    def begin(self, x: int, y: int) -> 'LineHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, ShapeEntry):
            cur_layer.line.begin = (x, y)
        return self

    def end(self, x: int, y: int) -> 'LineHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, ShapeEntry):
            cur_layer.line.end = (x, y)
        return self

    def style(self, style: LineStyle) -> 'LineHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, ShapeEntry):
            cur_layer.line.style = style
        return self


def rect(width: int, height: int, key: str = '') -> RectHandle:

    entry = ShapeEntry('RECT')
    entry.rect.width = width
    entry.rect.height = height
    idx = register_entry(entry, 'SHAPE', key)
    return RectHandle(idx)


def circle(radius: float, key: str = '') -> CircleHandle:

    entry = ShapeEntry('CIRCLE')
    entry.circle.circle_radius = radius
    idx = register_entry(entry, 'SHAPE', key)
    return CircleHandle(idx)


def tri(side: float, key: str = '') -> TriangleHandle:

    entry = ShapeEntry('TRIANGLE')
    entry.tri.side = side
    idx = register_entry(entry, 'SHAPE', key)
    return TriangleHandle(idx)


def line(size: float, key: str = '') -> LineHandle:

    entry = ShapeEntry('LINE')
    entry.line.size = size
    idx = register_entry(entry, 'SHAPE', key)
    return LineHandle(idx)
