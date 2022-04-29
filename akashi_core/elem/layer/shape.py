# pyright: reportPrivateUsage=false
from __future__ import annotations
from dataclasses import dataclass, field
import typing as tp
from typing import runtime_checkable

from akashi_core.elem.context import lcenter
from akashi_core.color import Color as ColorEnum
from akashi_core.color import color_value
from .base import (
    TransformField,
    TransformTrait,
    ShaderField,
    LayerField,
    LayerTrait
)
from .base import peek_entry, register_entry, frag, poly, LayerRef
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


@dataclass
class ShapeLocalField:
    shape_kind: ShapeKind
    fill: bool = True
    color: str = ""  # "#rrggbb" or "#rrggbbaa"
    border_size: float = 0
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

    def edge_radius(self, radius: float) -> 'ShapeLocalTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(HasShapeLocalField, cur_layer).shape.edge_radius = radius
        return self


@dataclass
class ShapeEntry(LayerField, RequiredParams):

    shape: ShapeLocalField = field(init=False)
    transform: TransformField = field(init=False)
    shader: ShaderField = field(init=False)

    def __post_init__(self):

        self.shape = ShapeLocalField(self._req_shape_kind)
        self.transform = TransformField()
        self.shader = ShaderField()


@dataclass
class ShapeHandle(LayerTrait):

    shape: ShapeLocalTrait = field(init=False)
    transform: TransformTrait = field(init=False)

    def __post_init__(self):
        self.shape = ShapeLocalTrait(self._idx)
        self.transform = TransformTrait(self._idx)

    def frag(self, *frag_fns: _ShapeFragFn, preamble: tuple[str, ...] = tuple()) -> 'ShapeHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, ShapeEntry):
            cur_layer.shader.frag_shader = ShaderCompiler(frag_fns, ShapeFragBuffer, _frag_shader_header, preamble)
        return self

    def poly(self, *poly_fns: _ShapePolyFn, preamble: tuple[str, ...] = tuple()) -> 'ShapeHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, ShapeEntry):
            cur_layer.shader.poly_shader = ShaderCompiler(poly_fns, ShapePolyBuffer, _poly_shader_header, preamble)
        return self


@dataclass
class RectTrait(ShapeHandle):
    ...


@dataclass
class CircleTrait(ShapeHandle):

    def lod(self, value: int) -> 'CircleTrait':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, ShapeEntry):
            cur_layer.shape.circle.lod = value
        return self


@dataclass
class TriangleTrait(ShapeHandle):
    ...


@dataclass
class LineTrait(ShapeHandle):

    def begin(self, x: int, y: int) -> 'LineTrait':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, ShapeEntry):
            cur_layer.shape.line.begin = (x, y)
        return self

    def end(self, x: int, y: int) -> 'LineTrait':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, ShapeEntry):
            cur_layer.shape.line.end = (x, y)
        return self

    def style(self, style: LineStyle) -> 'LineTrait':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, ShapeEntry):
            cur_layer.shape.line.style = style
        return self


shape_frag = ShapeFragBuffer

shape_poly = ShapePolyBuffer

rect_frag = ShapeFragBuffer

rect_poly = ShapePolyBuffer

RectTraitFn = tp.Callable[[RectTrait], tp.Any]


def rect(width: int, height: int, trait_fn: RectTraitFn) -> LayerRef:

    entry = ShapeEntry('RECT')
    entry.shape.rect.width = width
    entry.shape.rect.height = height
    idx = register_entry(entry, 'SHAPE', '')
    t = RectTrait(idx)
    t.transform.pos(*lcenter())
    trait_fn(t)
    return LayerRef(idx)


circle_frag = ShapeFragBuffer

circle_poly = ShapePolyBuffer

CircleTraitFn = tp.Callable[[CircleTrait], tp.Any]


def circle(radius: float, trait_fn: CircleTraitFn) -> LayerRef:

    entry = ShapeEntry('CIRCLE')
    entry.shape.circle.circle_radius = radius
    idx = register_entry(entry, 'SHAPE', '')
    t = CircleTrait(idx)
    t.transform.pos(*lcenter())
    trait_fn(t)
    return LayerRef(idx)


tri_frag = ShapeFragBuffer

tri_poly = ShapePolyBuffer

TriangleTraitFn = tp.Callable[[TriangleTrait], tp.Any]


def tri(side: float, trait_fn: TriangleTraitFn) -> LayerRef:

    entry = ShapeEntry('TRIANGLE')
    entry.shape.tri.side = side
    idx = register_entry(entry, 'SHAPE', '')
    t = TriangleTrait(idx)
    t.transform.pos(*lcenter())
    trait_fn(t)
    return LayerRef(idx)


line_frag = ShapeFragBuffer

line_poly = ShapePolyBuffer

LineTraitFn = tp.Callable[[LineTrait], tp.Any]


def line(size: float, trait_fn: LineTraitFn) -> LayerRef:

    entry = ShapeEntry('LINE')
    entry.shape.line.size = size
    idx = register_entry(entry, 'SHAPE', '')
    t = LineTrait(idx)
    t.transform.pos(*lcenter())
    trait_fn(t)
    return LayerRef(idx)
