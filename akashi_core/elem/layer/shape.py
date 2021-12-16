# pyright: reportPrivateUsage=false
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

_TShapeLayer = tp.TypeVar('_TShapeLayer', bound='ShapeTrait')


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

    def fill(self: '_TShapeLayer', enable_fill: bool) -> '_TShapeLayer':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(ShapeField, cur_layer).fill = enable_fill
        return self

    def color(self: '_TShapeLayer', color: tp.Union[str, 'ColorEnum']) -> '_TShapeLayer':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(ShapeField, cur_layer).color = color_value(color)
        return self

    def border_size(self: '_TShapeLayer', size: float) -> '_TShapeLayer':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(ShapeField, cur_layer).border_size = size
        return self

    def edge_radius(self: '_TShapeLayer', radius: float) -> '_TShapeLayer':
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
    ...


@dataclass
class CircleHandle(FittableDurationTrait, ShaderTrait, PositionTrait, ShapeTrait, LayerTrait):

    def lod(self, value: int) -> 'CircleHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, ShapeEntry):
            cur_layer.circle.lod = value
        return self


@dataclass
class TriangleHandle(FittableDurationTrait, ShaderTrait, PositionTrait, ShapeTrait, LayerTrait):
    ...


@dataclass
class LineHandle(FittableDurationTrait, ShaderTrait, PositionTrait, ShapeTrait, LayerTrait):

    def color(self, color: tp.Union[str, 'ColorEnum']) -> 'LineHandle':
        return super().color(color)

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
