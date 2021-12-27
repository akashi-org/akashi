# pyright: reportPrivateUsage=false
from __future__ import annotations
from dataclasses import dataclass, field
import typing as tp
from abc import ABCMeta

from akashi_core.color import Color as ColorEnum
from akashi_core.color import color_value
from .base import (
    FittableDurationTrait,
    PositionField,
    PositionTrait,
    ShaderField,
    LayerField,
    LayerTrait
)
from .base import peek_entry, register_entry, frag, poly
from akashi_core.pysl import _gl as gl
from akashi_core.pysl.shader import ShaderCompiler, _frag_shader_header, _poly_shader_header
from akashi_core.pysl.shader import LEntryFragFn, LEntryPolyFn
from akashi_core.pysl.shader import _NamedEntryFragFn, _NamedEntryPolyFn, _TEntryFnOpaque


@dataclass
class ShapeUniform:
    texture0: tp.Final[gl.uniform[gl.sampler2D]] = gl._uniform_default()


@dataclass
class ShapeFragBuffer(frag, ShapeUniform, gl._LayerFragInput):
    ...


@dataclass
class ShapePolyBuffer(poly, ShapeUniform, gl._LayerPolyOutput):
    ...


_ShapeFragFn = LEntryFragFn[ShapeFragBuffer] | _TEntryFnOpaque[_NamedEntryFragFn[ShapeFragBuffer]]
_ShapePolyFn = LEntryPolyFn[ShapePolyBuffer] | _TEntryFnOpaque[_NamedEntryPolyFn[ShapePolyBuffer]]


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

    def frag(self: '_TShapeLayer', *frag_fns: _ShapeFragFn, preamble: tuple[str, ...] = tuple()) -> '_TShapeLayer':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, ShapeEntry):
            cur_layer.frag_shader = ShaderCompiler(frag_fns, ShapeFragBuffer, _frag_shader_header, preamble)
        return self

    def poly(self: '_TShapeLayer', *poly_fns: _ShapePolyFn, preamble: tuple[str, ...] = tuple()) -> '_TShapeLayer':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, ShapeEntry):
            cur_layer.poly_shader = ShaderCompiler(poly_fns, ShapePolyBuffer, _poly_shader_header, preamble)
        return self

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
class RectHandle(FittableDurationTrait, PositionTrait, ShapeTrait, LayerTrait):
    ...


@dataclass
class CircleHandle(FittableDurationTrait, PositionTrait, ShapeTrait, LayerTrait):

    def lod(self, value: int) -> 'CircleHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, ShapeEntry):
            cur_layer.circle.lod = value
        return self


@dataclass
class TriangleHandle(FittableDurationTrait, PositionTrait, ShapeTrait, LayerTrait):
    ...


@dataclass
class LineHandle(FittableDurationTrait, PositionTrait, ShapeTrait, LayerTrait):

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


class rect(object):

    frag: tp.ClassVar[tp.Type[ShapeFragBuffer]] = ShapeFragBuffer
    poly: tp.ClassVar[tp.Type[ShapePolyBuffer]] = ShapePolyBuffer

    def __new__(cls, width: int, height: int, key: str = '') -> RectHandle:

        entry = ShapeEntry('RECT')
        entry.rect.width = width
        entry.rect.height = height
        idx = register_entry(entry, 'SHAPE', key)
        return RectHandle(idx)


class circle(object):

    frag: tp.ClassVar[tp.Type[ShapeFragBuffer]] = ShapeFragBuffer
    poly: tp.ClassVar[tp.Type[ShapePolyBuffer]] = ShapePolyBuffer

    def __new__(cls, radius: float, key: str = '') -> CircleHandle:

        entry = ShapeEntry('CIRCLE')
        entry.circle.circle_radius = radius
        idx = register_entry(entry, 'SHAPE', key)
        return CircleHandle(idx)


class tri(object):

    frag: tp.ClassVar[tp.Type[ShapeFragBuffer]] = ShapeFragBuffer
    poly: tp.ClassVar[tp.Type[ShapePolyBuffer]] = ShapePolyBuffer

    def __new__(cls, side: float, key: str = '') -> TriangleHandle:

        entry = ShapeEntry('TRIANGLE')
        entry.tri.side = side
        idx = register_entry(entry, 'SHAPE', key)
        return TriangleHandle(idx)


class line(object):

    frag: tp.ClassVar[tp.Type[ShapeFragBuffer]] = ShapeFragBuffer
    poly: tp.ClassVar[tp.Type[ShapePolyBuffer]] = ShapePolyBuffer

    def __new__(cls, size: float, key: str = '') -> LineHandle:

        entry = ShapeEntry('LINE')
        entry.line.size = size
        idx = register_entry(entry, 'SHAPE', key)
        return LineHandle(idx)
