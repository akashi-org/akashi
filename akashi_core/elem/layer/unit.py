# pyright: reportPrivateUsage=false, reportIncompatibleMethodOverride=false
from __future__ import annotations
from dataclasses import dataclass, field
import typing as tp
from typing import runtime_checkable, overload

from .base import (
    ShaderField,
    LayerField,
    LayerTrait,
    HasTransformField,
    TransformField,
    TransformTrait,
    TextureField,
    TextureTrait,
)
from .base import register_entry, peek_entry, frag, poly, HasMediaField, LayerRef
from akashi_core.pysl import _gl as gl
from akashi_core.time import sec, NOT_FIXED_SEC
from akashi_core.probe import get_duration, g_resource_map
from akashi_core.pysl.shader import (
    ShaderCompiler,
    _frag_shader_header,
    _poly_shader_header,
    _NamedEntryFragFn,
    _NamedEntryPolyFn,
    _TEntryFnOpaque,
    ShaderMemoType
)
from akashi_core.elem.context import _GlobalKronContext as gctx
from akashi_core.elem.context import lwidth as ak_lwidth
from akashi_core.elem.context import lheight as ak_lheight
from akashi_core.elem.context import lcenter as ak_lcenter
from akashi_core.color import Color as ColorEnum
from akashi_core.color import color_value

from akashi_core.elem.layout import LayoutFn, LayoutInfo, LayoutLayerContext


_FrameFnParams = tp.ParamSpec('_FrameFnParams')
# FrameKind = tp.Literal['spatial', 'timeline', 'directive']
FrameKind = tp.Literal['spatial', 'timeline']
_FrameFnCtx = tuple[FrameKind, tp.Callable[[], None]]

_T_FrameKind = tp.TypeVar('_T_FrameKind', bound=FrameKind)


class _FrameFnCtxOpaque(tp.Generic[_T_FrameKind]):
    ...


def frame(kind: _T_FrameKind = 'spatial') -> tp.Callable[[tp.Callable[_FrameFnParams, None]], tp.Callable[_FrameFnParams, _FrameFnCtxOpaque[_T_FrameKind]]]:

    def inner(f: tp.Callable[_FrameFnParams, None]) -> tp.Callable[_FrameFnParams, _FrameFnCtx]:
        def wrapper(*args: _FrameFnParams.args, **kwargs: _FrameFnParams.kwargs) -> _FrameFnCtx:
            return (kind, lambda: f(*args, **kwargs))
        return wrapper

    return tp.cast(tp.Callable[[tp.Callable[_FrameFnParams, None]], tp.Callable[_FrameFnParams, _FrameFnCtxOpaque[_T_FrameKind]]], inner)


class UnitUniform:
    texture0: tp.Final[gl.uniform[gl.sampler2D]] = gl._uniform_default()


@dataclass
class UnitFragBuffer(frag, UnitUniform, gl._LayerFragInput):
    ...


@dataclass
class UnitPolyBuffer(poly, UnitUniform, gl._LayerPolyOutput):
    ...


@dataclass
class UnitLocalField:
    layer_indices: list[int] = field(default_factory=list, init=False)
    fb_size: tuple[int, int] = field(default=(0, 0), init=False)
    bg_color: str = "#00000000"  # transparent
    _layout_fn: LayoutFn | None = None


@runtime_checkable
class HasUnitLocalField(tp.Protocol):
    unit: UnitLocalField


@dataclass
class UnitEntry(LayerField):

    unit: UnitLocalField = field(init=False)
    transform: TransformField = field(init=False)
    tex: TextureField = field(init=False)
    shader: ShaderField = field(init=False)

    def __post_init__(self):
        self.unit = UnitLocalField()
        self.transform = TransformField()
        self.tex = TextureField()
        self.shader = ShaderField()


_UnitFragFn = _TEntryFnOpaque[_NamedEntryFragFn[UnitFragBuffer]]
_UnitPolyFn = _TEntryFnOpaque[_NamedEntryPolyFn[UnitPolyBuffer]]


@dataclass
class SpatialFrameTrait(LayerTrait):

    transform: TransformTrait = field(init=False)
    tex: TextureTrait = field(init=False)

    def __post_init__(self):
        self.tex = TextureTrait(self._idx)
        self.transform = TransformTrait(self._idx)

    def __enter__(self) -> 'SpatialFrameTrait':
        gctx.get_ctx()._cur_unit_ids.append(self._idx)
        return self

    def __exit__(self, *ext: tp.Any):
        cur_ctx = gctx.get_ctx()
        cur_unit_layer = tp.cast(UnitEntry, cur_ctx.layers[cur_ctx._cur_unit_ids[-1]])

        if isinstance(cur_unit_layer.duration, sec):
            max_to: sec = sec(0)
            for layout_idx, layer_idx in enumerate(cur_unit_layer.unit.layer_indices):

                cur_layer = cur_ctx.layers[layer_idx]

                if isinstance(cur_layer, HasTransformField) and cur_unit_layer.unit._layout_fn:

                    layout_info = cur_unit_layer.unit._layout_fn(LayoutLayerContext(
                        cur_layer.key,
                        layout_idx,
                        len(cur_unit_layer.unit.layer_indices)
                    ))
                    if layout_info:

                        if layout_info.pos:
                            cur_layer.transform.pos = layout_info.pos

                        if layout_info.z:
                            cur_layer.transform.z = layout_info.z

                        if layout_info.layer_size and (temp_layer_size := list(layout_info.layer_size)):
                            if cur_layer.kind == 'UNIT':
                                aspect_ratio = sec(tp.cast(UnitEntry, cur_layer).unit.fb_size[0], tp.cast(
                                    UnitEntry, cur_layer).unit.fb_size[1])
                                if temp_layer_size[0] == -1:
                                    temp_layer_size[0] = (sec(temp_layer_size[1]) * aspect_ratio).trunc()
                                if temp_layer_size[1] == -1:
                                    temp_layer_size[1] = (sec(temp_layer_size[0]) / aspect_ratio).trunc()
                            cur_layer.transform.layer_size = (temp_layer_size[0], temp_layer_size[1])

                if isinstance(cur_layer._duration, sec):

                    if cur_layer.duration == NOT_FIXED_SEC:
                        cur_ctx.layers[layer_idx].duration = cur_layer._duration

                    layer_to = cur_ctx.layers[layer_idx].atom_offset + tp.cast(sec, cur_ctx.layers[layer_idx].duration)
                    if layer_to > max_to:
                        max_to = layer_to
                else:  # assumes AtomHandle
                    raise Exception('Passing a layer handle to duration is prohibited for child layers of an unit layer')  # noqa: E501

            cur_unit_layer.duration = max_to

        cur_ctx._cur_unit_ids.pop()
        return False

    def layout(self, layout_fn: LayoutFn) -> 'SpatialFrameTrait':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, UnitEntry):
            cur_layer.unit._layout_fn = layout_fn
        return self

    def frag(self, *frag_fns: _UnitFragFn, preamble: tuple[str, ...] = tuple(), memo: ShaderMemoType = None) -> 'SpatialFrameTrait':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, UnitEntry):
            cur_layer.shader.frag_shader = ShaderCompiler(frag_fns, UnitFragBuffer, _frag_shader_header, preamble, memo)
        return self

    def poly(self, *poly_fns: _UnitPolyFn, preamble: tuple[str, ...] = tuple(), memo: ShaderMemoType = None) -> 'SpatialFrameTrait':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, UnitEntry):
            cur_layer.shader.poly_shader = ShaderCompiler(poly_fns, UnitPolyBuffer, _poly_shader_header, preamble, memo)
        return self

    def bg_color(self, color: tp.Union[str, 'ColorEnum']) -> 'SpatialFrameTrait':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, UnitEntry):
            cur_layer.unit.bg_color = color_value(color)
        return self

    def fb_size(self, width: int, height: int, copy_to_layer_size: bool = True) -> 'SpatialFrameTrait':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, UnitEntry):
            cur_layer.unit.fb_size = (width, height)
            if copy_to_layer_size:
                self.transform.layer_size(*cur_layer.unit.fb_size)
        return self


unit_frag = UnitFragBuffer

unit_poly = UnitPolyBuffer


@dataclass
class TimelineFrameTrait(LayerTrait):

    transform: TransformTrait = field(init=False)

    def __post_init__(self):
        self.transform = TransformTrait(self._idx)

    @staticmethod
    def __update_unit_layer_atom_offset(unit_entry: 'UnitEntry', new_offset: sec):

        cur_ctx = gctx.get_ctx()
        unit_entry.atom_offset += new_offset
        for layer_idx in unit_entry.unit.layer_indices:
            cur_layer = cur_ctx.layers[layer_idx]
            if cur_layer.kind == 'UNIT':
                TimelineFrameTrait.__update_unit_layer_atom_offset(tp.cast('UnitEntry', cur_layer), new_offset)
            else:
                cur_layer.atom_offset += new_offset

    def __enter__(self) -> 'TimelineFrameTrait':
        gctx.get_ctx()._cur_unit_ids.append(self._idx)
        return self

    def __exit__(self, *ext: tp.Any):
        cur_ctx = gctx.get_ctx()
        cur_unit_layer = tp.cast(UnitEntry, cur_ctx.layers[cur_ctx._cur_unit_ids[-1]])

        if not isinstance(cur_unit_layer.duration, sec):
            raise Exception('Passing a layer handle to duration is prohibited for an scene layer')

        acc_duration: sec = sec(0)
        for layer_idx in cur_unit_layer.unit.layer_indices:

            cur_layer = cur_ctx.layers[layer_idx]
            if cur_layer.kind == 'UNIT':
                TimelineFrameTrait.__update_unit_layer_atom_offset(tp.cast('UnitEntry', cur_layer), acc_duration)
            else:
                cur_layer.atom_offset += acc_duration

            if not isinstance(cur_layer._duration, sec):
                raise Exception('Passing a layer handle to duration is prohibited for child layers of an scene layer')

            if cur_layer.duration == NOT_FIXED_SEC:
                cur_layer.duration = cur_layer._duration

            acc_duration += cur_layer.duration

        cur_unit_layer.duration = acc_duration

        cur_ctx._cur_unit_ids.pop()
        return False

    def bg_color(self, color: tp.Union[str, 'ColorEnum']) -> 'TimelineFrameTrait':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, UnitEntry):
            cur_layer.unit.bg_color = color_value(color)
        return self

    def fb_size(self, width: int, height: int, copy_to_layer_size: bool = True) -> 'TimelineFrameTrait':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, UnitEntry):
            cur_layer.unit.fb_size = (width, height)
            if copy_to_layer_size:
                self.transform.layer_size(*cur_layer.unit.fb_size)
        return self


@overload
def unit(frame_ctx: _FrameFnCtxOpaque[tp.Literal['spatial']], *trait_fns: tp.Callable[[SpatialFrameTrait], tp.Any]) -> LayerRef:
    ...


@overload
def unit(frame_ctx: _FrameFnCtxOpaque[tp.Literal['timeline']], *trait_fns: tp.Callable[[TimelineFrameTrait], tp.Any]) -> LayerRef:
    ...


@overload
def unit(frame_ctx: tp.Never, *trait_fns: tp.Never) -> tp.Never:
    ...


def unit(frame_ctx, *trait_fns) -> LayerRef:

    kind, layer_fns = tp.cast(_FrameFnCtx, frame_ctx)
    if kind == 'spatial':
        entry = UnitEntry()
        entry.transform.layer_size = (ak_lwidth(), ak_lheight())
        entry.unit.fb_size = entry.transform.layer_size
        idx = register_entry(entry, 'UNIT', '')

        h = SpatialFrameTrait(idx)
        h.transform.pos(*ak_lcenter())

        with h:
            [tfn(h) for tfn in trait_fns]
            layer_fns()

        return LayerRef(idx)
    elif kind == 'timeline':
        entry = UnitEntry()
        entry.transform.layer_size = (ak_lwidth(), ak_lheight())
        entry.unit.fb_size = entry.transform.layer_size
        idx = register_entry(entry, 'UNIT', '')

        h = TimelineFrameTrait(idx)
        h.transform.pos(*ak_lcenter())

        with h:
            [tfn(h) for tfn in trait_fns]
            layer_fns()

        return LayerRef(idx)
    else:
        raise NotImplementedError()
