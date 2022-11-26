# pyright: reportPrivateUsage=false
from __future__ import annotations
from dataclasses import dataclass, field
import typing as tp
from typing import runtime_checkable, overload

from akashi_core.elem.context import lcenter, lwidth as ak_lwidth, lheight as ak_lheight
from akashi_core.color import Color as ColorEnum
from akashi_core.color import color_value
from .base import (
    TransformField,
    TransformTrait,
    TextureField,
    TextureTrait,
    ShaderField,
    LayerField,
    LayerTrait,
    LayerTimeTrait,
)
from .base import peek_entry, register_entry, frag, poly, LayerRef
from akashi_core.pysl import _gl as gl
from akashi_core.pysl.shader import ShaderCompiler, _frag_shader_header, _poly_shader_header
from akashi_core.pysl.shader import _NamedEntryFragFn, _NamedEntryPolyFn, _TEntryFnOpaque, ShaderMemoType

import math


@dataclass
class ShapeUniform:
    texture0: tp.Final[gl.uniform[gl.sampler2D]] = gl._uniform_default()


@dataclass
class ShapeFragBuffer(frag, ShapeUniform, gl._LayerFragInput):
    ...


@dataclass
class ShapePolyBuffer(poly, ShapeUniform, gl._LayerPolyOutput):
    ...


_ShapeFragFn = _TEntryFnOpaque[_NamedEntryFragFn[ShapeFragBuffer]]
_ShapePolyFn = _TEntryFnOpaque[_NamedEntryPolyFn[ShapePolyBuffer]]


@dataclass
class RectDetail:
    width: int = 0
    height: int = 0


@dataclass
class CircleDetail:
    ...


@dataclass
class TriangleDetail:
    width: int = 0
    height: int = 0
    wr: float = 0.0
    hr: float = 1.0  # range: 0 <= hr <= 1.0


LineStyle = tp.Literal['default', 'round-dot', 'square-dot', 'cap']


@dataclass
class LineDetail:
    size: float = 0
    begin: tuple[int, int] = (0, 0)
    end: tuple[int, int] = (0, 0)
    style: LineStyle = 'default'


''' Shape Concept '''

ShapeKind = tp.Literal['RECT', 'CIRCLE', 'TRIANGLE', 'LINE']

BorderDirection = tp.Literal['inner', 'outer', 'full']


@dataclass
class ShapeLocalField:
    shape_kind: ShapeKind
    fill: bool = True
    color: str = ""  # "#rrggbb" or "#rrggbbaa"
    border_size: float = 0
    border_color: str = ""  # "#rrggbb" or "#rrggbbaa"
    border_direction: BorderDirection = 'inner'
    edge_radius: float = 0
    rect: RectDetail = field(init=False)
    circle: CircleDetail = field(init=False)
    tri: TriangleDetail = field(init=False)
    line: LineDetail = field(init=False)

    def __post_init__(self):

        self.rect = RectDetail()
        self.circle = CircleDetail()
        self.tri = TriangleDetail()
        self.line = LineDetail()


@runtime_checkable
class HasShapeLocalField(tp.Protocol):
    shape: ShapeLocalField


@dataclass
class RequiredParams:
    _req_shape_kind: ShapeKind


@dataclass
class ShapeLocalTrait:

    _idx: int

    def fill(self, enable_fill: bool) -> 'ShapeLocalTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(HasShapeLocalField, cur_layer).shape.fill = enable_fill
        return self

    def color(self, color: tp.Union[str, 'ColorEnum']) -> 'ShapeLocalTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(HasShapeLocalField, cur_layer).shape.color = color_value(color)
        return self

    def border_size(self, size: float) -> 'ShapeLocalTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(HasShapeLocalField, cur_layer).shape.border_size = size
        return self

    def border_color(self, color: tp.Union[str, 'ColorEnum']) -> 'ShapeLocalTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(HasShapeLocalField, cur_layer).shape.border_color = color_value(color)
        return self

    def border_direction(self, direction: BorderDirection) -> 'ShapeLocalTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(HasShapeLocalField, cur_layer).shape.border_direction = direction
        return self

    def edge_radius(self, radius: float) -> 'ShapeLocalTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(HasShapeLocalField, cur_layer).shape.edge_radius = radius
        return self


@dataclass
class ShapeEntry(LayerField, RequiredParams):

    shape: ShapeLocalField = field(init=False)
    transform: TransformField = field(init=False)
    shader: ShaderField = field(init=False)
    tex: TextureField = field(init=False)

    def __post_init__(self):

        self.shape = ShapeLocalField(self._req_shape_kind)
        self.transform = TransformField()
        self.shader = ShaderField()
        self.tex = TextureField()


@dataclass
class ShapeTrait(LayerTrait, LayerTimeTrait):

    shape: ShapeLocalTrait = field(init=False)
    transform: TransformTrait = field(init=False)
    tex: TextureTrait = field(init=False)

    def __post_init__(self):
        self.shape = ShapeLocalTrait(self._idx)
        self.transform = TransformTrait(self._idx)
        self.tex = TextureTrait(self._idx)

    def frag(self, *frag_fns: _ShapeFragFn, preamble: tuple[str, ...] = tuple(), memo: ShaderMemoType = None) -> 'ShapeTrait':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, ShapeEntry):
            cur_layer.shader.frag_shader = ShaderCompiler(
                frag_fns, ShapeFragBuffer, _frag_shader_header, preamble, memo)
        return self

    def poly(self, *poly_fns: _ShapePolyFn, preamble: tuple[str, ...] = tuple(), memo: ShaderMemoType = None) -> 'ShapeTrait':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, ShapeEntry):
            cur_layer.shader.poly_shader = ShaderCompiler(
                poly_fns, ShapePolyBuffer, _poly_shader_header, preamble, memo)
        return self


@dataclass
class RectTrait(ShapeTrait):
    ...


@dataclass
class CircleTrait(ShapeTrait):
    ...


@dataclass
class TriangleTrait(ShapeTrait):
    ...


@dataclass
class LineLocalTrait:

    _idx: int

    def begin(self, x: int, y: int) -> 'LineLocalTrait':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, ShapeEntry):
            cur_layer.shape.line.begin = (x, y)
        return self

    def end(self, x: int, y: int) -> 'LineLocalTrait':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, ShapeEntry):
            cur_layer.shape.line.end = (x, y)
        return self

    # [TODO] Impl later
    # def style(self, style: LineStyle) -> 'LineLocalTrait':
    #     if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, ShapeEntry):
    #         cur_layer.shape.line.style = style
    #     return self


@dataclass
class LineTrait(ShapeTrait):

    line: LineLocalTrait


shape_frag = ShapeFragBuffer

shape_poly = ShapePolyBuffer

rect_frag = ShapeFragBuffer

rect_poly = ShapePolyBuffer

RectTraitFn = tp.Callable[[RectTrait], tp.Any]


def rect(width: int, height: int, *trait_fns: RectTraitFn) -> LayerRef:

    entry = ShapeEntry('RECT')
    entry.shape.rect.width = width
    entry.shape.rect.height = height
    idx = register_entry(entry, 'SHAPE', '')
    t = RectTrait(idx)
    t.transform.pos(*lcenter())
    [tfn(t) for tfn in trait_fns]
    return LayerRef(idx)


circle_frag = ShapeFragBuffer

circle_poly = ShapePolyBuffer

CircleTraitFn = tp.Callable[[CircleTrait], tp.Any]


@overload
def circle(size: int, *trait_fns: CircleTraitFn) -> LayerRef:
    ...


@overload
def circle(size: tuple[int, int], *trait_fns: CircleTraitFn) -> LayerRef:
    ...


def circle(size: int | tuple[int, int], *trait_fns: CircleTraitFn) -> LayerRef:

    entry = ShapeEntry('CIRCLE')

    if isinstance(size, tuple):
        entry.transform.layer_size = size
    else:
        entry.transform.layer_size = (size, size)

    idx = register_entry(entry, 'SHAPE', '')
    t = CircleTrait(idx)
    t.transform.pos(*lcenter())
    [tfn(t) for tfn in trait_fns]
    return LayerRef(idx)


tri_frag = ShapeFragBuffer

tri_poly = ShapePolyBuffer

TriangleTraitFn = tp.Callable[[TriangleTrait], tp.Any]


@overload
def tri(size: int, *trait_fns: TriangleTraitFn) -> LayerRef:
    ...


@overload
def tri(size: tuple[int, int, float], *trait_fns: TriangleTraitFn) -> LayerRef:
    ...


@overload
def tri(size: tuple[int, int, float, float], *trait_fns: TriangleTraitFn) -> LayerRef:
    ...


def tri(size: int | tuple[int, int, float] | tuple[int, int, float, float], *trait_fns: TriangleTraitFn) -> LayerRef:

    entry = ShapeEntry('TRIANGLE')

    if isinstance(size, tuple):
        entry.shape.tri.width = size[0]
        entry.shape.tri.height = size[1]
        entry.shape.tri.wr = size[2]
        if len(size) > 3:
            _size = tp.cast(tuple[int, int, float, float], size)
            entry.shape.tri.hr = 0 if (_size[3] < 0 or _size[3] > 1) else _size[3]
    else:
        entry.shape.tri.width = size
        entry.shape.tri.height = int(size * 0.5 * math.sqrt(3))
        entry.shape.tri.wr = 0.5

    idx = register_entry(entry, 'SHAPE', '')
    t = TriangleTrait(idx)
    t.transform.pos(*lcenter())
    [tfn(t) for tfn in trait_fns]
    return LayerRef(idx)


line_frag = ShapeFragBuffer

line_poly = ShapePolyBuffer

LineTraitFn = tp.Callable[[LineTrait], tp.Any]


def line(size: float, *trait_fns: LineTraitFn) -> LayerRef:

    entry = ShapeEntry('LINE')
    entry.shape.line.size = size
    idx = register_entry(entry, 'SHAPE', '')
    t = LineTrait(idx, LineLocalTrait(idx))
    t.transform.pos(*lcenter())
    t.transform.layer_size(ak_lwidth(), ak_lheight())
    [tfn(t) for tfn in trait_fns]
    return LayerRef(idx)
