# pyright: reportPrivateUsage=false, reportIncompatibleMethodOverride=false
from __future__ import annotations
from dataclasses import dataclass, field
import typing as tp
from typing import runtime_checkable, overload
import copy

from .base import (
    ShaderField,
    LayerField,
    LayerTrait,
    HasTransformField,
    HasMediaField,
    HasShaderField,
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

if tp.TYPE_CHECKING:
    from akashi_core.elem.context import KronContext


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
    _start: sec = field(default_factory=lambda: sec(0))
    _end: sec = field(default_factory=lambda: sec(-1))
    _layout_fn: LayoutFn | None = None
    _span_cnt: int | None = None
    _span_dur: sec | None = None


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
class SpatialUnitTrait(LayerTrait):

    transform: TransformTrait = field(init=False)
    tex: TextureTrait = field(init=False)

    def __post_init__(self):
        self.tex = TextureTrait(self._idx)
        self.transform = TransformTrait(self._idx)

    def layout(self, layout_fn: LayoutFn) -> 'SpatialUnitTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(HasUnitLocalField, cur_layer).unit._layout_fn = layout_fn
        return self

    def frag(self, *frag_fns: _UnitFragFn, preamble: tuple[str, ...] = tuple(), memo: ShaderMemoType = None) -> 'SpatialUnitTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(HasShaderField, cur_layer).shader.frag_shader = ShaderCompiler(
                frag_fns, UnitFragBuffer, _frag_shader_header, preamble, memo)
        return self

    def poly(self, *poly_fns: _UnitPolyFn, preamble: tuple[str, ...] = tuple(), memo: ShaderMemoType = None) -> 'SpatialUnitTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(HasShaderField, cur_layer).shader.poly_shader = ShaderCompiler(
                poly_fns, UnitPolyBuffer, _poly_shader_header, preamble, memo)
        return self

    def bg_color(self, color: tp.Union[str, 'ColorEnum']) -> 'SpatialUnitTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(HasUnitLocalField, cur_layer).unit.bg_color = color_value(color)
        return self

    def fb_size(self, width: int, height: int, copy_to_layer_size: bool = True) -> 'SpatialUnitTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(HasUnitLocalField, cur_layer).unit.fb_size = (width, height)
            if copy_to_layer_size:
                self.transform.layer_size(*tp.cast(HasUnitLocalField, cur_layer).unit.fb_size)
        return self

    def range(self, start: sec | float, end: sec | float = -1) -> 'SpatialUnitTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(HasUnitLocalField, cur_layer).unit._start = sec(start)
            tp.cast(HasUnitLocalField, cur_layer).unit._end = sec(end)
        return self

    def span_cnt(self, count: int) -> 'SpatialUnitTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(HasUnitLocalField, cur_layer).unit._span_cnt = count
            tp.cast(HasUnitLocalField, cur_layer).unit._span_dur = None
        return self


unit_frag = UnitFragBuffer

unit_poly = UnitPolyBuffer


@dataclass
class TimelineUnitTrait(LayerTrait):

    transform: TransformTrait = field(init=False)

    def __post_init__(self):
        self.transform = TransformTrait(self._idx)

    def bg_color(self, color: tp.Union[str, 'ColorEnum']) -> 'TimelineUnitTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(HasUnitLocalField, cur_layer).unit.bg_color = color_value(color)
        return self

    def fb_size(self, width: int, height: int, copy_to_layer_size: bool = True) -> 'TimelineUnitTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(HasUnitLocalField, cur_layer).unit.fb_size = (width, height)
            if copy_to_layer_size:
                self.transform.layer_size(*tp.cast(HasUnitLocalField, cur_layer).unit.fb_size)
        return self


@dataclass
class SpatialFrameGuard:

    _layer_idx: int

    def __enter__(self):
        gctx.get_ctx()._cur_unit_ids.append(self._layer_idx)
        return self

    def __exit__(self, *ext: tp.Any):
        cur_ctx = gctx.get_ctx()
        cur_unit_layer = tp.cast(UnitEntry, cur_ctx.layers[cur_ctx._cur_unit_ids[-1]])

        if not isinstance(cur_unit_layer._duration, sec):
            raise Exception('Invalid duration found')

        # Apply time/space layouts to its children
        max_to: sec = sec(0)
        for layout_idx, layer_idx in enumerate(cur_unit_layer.unit.layer_indices):
            cur_layer = cur_ctx.layers[layer_idx]
            self._apply_space_layout(cur_unit_layer, cur_layer, layout_idx)
            if (layer_to := self._apply_time_layout(cur_layer)) and layer_to > max_to:
                max_to = layer_to

        # Apply slice to its children and fix the duration
        self._apply_slice(cur_ctx, cur_unit_layer, max_to)

        # Span the slice and fix the duration
        self._apply_span(cur_ctx, cur_unit_layer)

        # Pop this frame
        cur_ctx._cur_unit_ids.pop()
        return False

    @staticmethod
    def _apply_space_layout(unit_layer: UnitEntry, child_layer: 'LayerField', layout_idx: int):

        if not isinstance(child_layer, HasTransformField):
            return

        if not unit_layer.unit._layout_fn:
            return

        layout_info = unit_layer.unit._layout_fn(LayoutLayerContext(
            child_layer.key,
            layout_idx,
            len(unit_layer.unit.layer_indices)
        ))

        if not layout_info:
            return

        if layout_info.pos:
            child_layer.transform.pos = layout_info.pos

        if layout_info.z:
            child_layer.transform.z = layout_info.z

        if layout_info.layer_size and (temp_layer_size := list(layout_info.layer_size)):
            if child_layer.kind == 'UNIT':
                aspect_ratio = sec(tp.cast(UnitEntry, child_layer).unit.fb_size[0], tp.cast(
                    UnitEntry, child_layer).unit.fb_size[1])
                if temp_layer_size[0] == -1:
                    temp_layer_size[0] = (sec(temp_layer_size[1]) * aspect_ratio).trunc()
                if temp_layer_size[1] == -1:
                    temp_layer_size[1] = (sec(temp_layer_size[0]) / aspect_ratio).trunc()
            child_layer.transform.layer_size = (temp_layer_size[0], temp_layer_size[1])

    @staticmethod
    def _apply_time_layout(child_layer: 'LayerField') -> sec:

        # AtomHandle case
        if not isinstance(child_layer._duration, sec):
            raise Exception('Passing a root handle to duration is prohibited within a frame')

        if child_layer.slice_offset == NOT_FIXED_SEC:
            child_layer.slice_offset = child_layer.frame_offset

        return child_layer.slice_offset + tp.cast(sec, child_layer._duration)

    @staticmethod
    def _apply_slice(cur_ctx: 'KronContext', unit_layer: UnitEntry, max_to: sec):

        if unit_layer.unit._start == sec(0) and unit_layer.unit._end == sec(-1):
            unit_layer._duration = max_to
            return

        unit_start = unit_layer.unit._start
        unit_end = max_to if unit_layer.unit._end == -1 else unit_layer.unit._end
        unit_dur = unit_end - unit_start
        living_layer_indices: list[int] = []

        for layer_idx in unit_layer.unit.layer_indices:
            cur_layer = cur_ctx.layers[layer_idx]
            layer_from = cur_layer.frame_offset
            layer_to = layer_from + tp.cast(sec, cur_layer._duration)
            if layer_from <= unit_end and layer_to >= unit_start:

                new_layer_local_offset: sec
                new_slice_offset: sec
                if unit_start < layer_from:
                    new_layer_local_offset = sec(0)
                    new_slice_offset = layer_from - unit_start
                else:
                    new_layer_local_offset = unit_start - layer_from
                    new_slice_offset = sec(0)

                # [XXX]
                # DO NOT directly compare frame values like layer_[from|to], unit_[start|end], with slice values like new_slice_offset
                new_layer_from = unit_start + new_slice_offset
                new_duration: sec
                if unit_end < layer_to:
                    new_duration = unit_end - new_layer_from
                else:
                    new_duration = layer_to - new_layer_from

                cur_layer.slice_offset = new_slice_offset
                cur_layer.layer_local_offset = new_layer_local_offset
                cur_layer._duration = new_duration

                if cur_layer.layer_local_offset > sec(0) and isinstance(cur_layer, HasMediaField):
                    media_slice_dur = cur_layer.media.end - cur_layer.media.start
                    media_slice_part = int(cur_layer.layer_local_offset / media_slice_dur)
                    cur_layer.layer_local_offset = cur_layer.layer_local_offset - (media_slice_part * media_slice_dur)

                living_layer_indices.append(layer_idx)
            else:
                cur_layer.defunct = True

        unit_layer.unit.layer_indices = living_layer_indices
        unit_layer._duration = unit_dur

    @staticmethod
    def _apply_span(cur_ctx: 'KronContext', unit_layer: UnitEntry):

        if unit_layer.unit._span_cnt and unit_layer.unit._span_cnt > 1:
            org_duration: sec = tp.cast(sec, unit_layer._duration)
            acc_duration: sec = org_duration
            child_layer_indices = copy.deepcopy(unit_layer.unit.layer_indices)
            for cnt in range(unit_layer.unit._span_cnt - 1):
                for layer_idx in child_layer_indices:
                    cur_layer = cur_ctx.layers[layer_idx]
                    if cur_layer.defunct:
                        continue
                    dup_layer = copy.deepcopy(cur_layer)
                    dup_layer.slice_offset += acc_duration
                    # [TODO] Should we rename the key?
                    # register_entry(dup_layer, dup_layer.kind, dup_layer.key + f'__span_cnt_{cnt+1}')
                    register_entry(dup_layer, dup_layer.kind, dup_layer.key)
                acc_duration += org_duration

            unit_layer._duration = tp.cast(sec, unit_layer._duration) * unit_layer.unit._span_cnt

        elif unit_layer.unit._span_dur:
            raise NotImplementedError()


@dataclass
class TimelineFrameGuard:

    _layer_idx: int

    def __enter__(self):
        gctx.get_ctx()._cur_unit_ids.append(self._layer_idx)
        return self

    def __exit__(self, *ext: tp.Any):

        cur_ctx = gctx.get_ctx()
        cur_unit_layer = tp.cast(UnitEntry, cur_ctx.layers[cur_ctx._cur_unit_ids[-1]])

        if not isinstance(cur_unit_layer._duration, sec):
            raise Exception('Invalid duration found')

        acc_duration: sec = sec(0)
        for layer_idx in cur_unit_layer.unit.layer_indices:

            cur_layer = cur_ctx.layers[layer_idx]
            if cur_layer.slice_offset == NOT_FIXED_SEC:
                cur_layer.slice_offset = cur_layer.frame_offset
            if cur_layer.kind == 'UNIT':
                self._update_unit_layer_slice_offset(tp.cast('UnitEntry', cur_layer), acc_duration)
            else:
                cur_layer.slice_offset += acc_duration

            if not isinstance(cur_layer._duration, sec):
                raise Exception('Passing a layer handle to duration is prohibited for child layers of an scene layer')

            acc_duration += cur_layer._duration

        cur_unit_layer._duration = acc_duration

        cur_ctx._cur_unit_ids.pop()
        return False

    @staticmethod
    def _update_unit_layer_slice_offset(unit_entry: 'UnitEntry', new_offset: sec):

        cur_ctx = gctx.get_ctx()
        unit_entry.slice_offset += new_offset
        for layer_idx in unit_entry.unit.layer_indices:
            cur_layer = cur_ctx.layers[layer_idx]
            if cur_layer.kind == 'UNIT':
                TimelineFrameGuard._update_unit_layer_slice_offset(tp.cast('UnitEntry', cur_layer), new_offset)
            else:
                cur_layer.slice_offset += new_offset


@overload
def unit(frame_ctx: _FrameFnCtxOpaque[tp.Literal['spatial']], *trait_fns: tp.Callable[[SpatialUnitTrait], tp.Any]) -> LayerRef:
    ...


@overload
def unit(frame_ctx: _FrameFnCtxOpaque[tp.Literal['timeline']], *trait_fns: tp.Callable[[TimelineUnitTrait], tp.Any]) -> LayerRef:
    ...


@overload
def unit(frame_ctx: tp.Never, *trait_fns: tp.Never) -> tp.Never:
    ...


def unit(frame_ctx, *trait_fns) -> LayerRef:

    kind, layer_fns = tp.cast(_FrameFnCtx, frame_ctx)

    if kind not in ['spatial', 'timeline']:
        raise NotImplementedError()

    entry = UnitEntry()
    entry.transform.layer_size = (ak_lwidth(), ak_lheight())
    entry.unit.fb_size = entry.transform.layer_size
    idx = register_entry(entry, 'UNIT', '')

    h: SpatialUnitTrait | TimelineUnitTrait
    guard: SpatialFrameGuard | TimelineFrameGuard
    if kind == 'spatial':
        h = SpatialUnitTrait(idx)
        guard = SpatialFrameGuard(idx)
    else:
        h = TimelineUnitTrait(idx)
        guard = TimelineFrameGuard(idx)

    h.transform.pos(*ak_lcenter())

    with guard:
        [tfn(h) for tfn in trait_fns]
        layer_fns()

    return LayerRef(idx)
