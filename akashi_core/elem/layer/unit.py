# pyright: reportPrivateUsage=false, reportIncompatibleMethodOverride=false
from __future__ import annotations
from dataclasses import dataclass, field
import typing as tp
from typing import runtime_checkable, overload
import copy

from .base import (
    _BaseTrait,
    _BaseTraitField,
    LayerRef,
    LayerField,
    register_layer_field,
)

from akashi_core.pysl import _gl as gl
from akashi_core.time import sec, NOT_FIXED_SEC
from akashi_core.probe import get_duration, g_resource_map
from akashi_core.elem.context import _GlobalKronContext as gctx
from akashi_core.elem.context import lwidth as ak_lwidth
from akashi_core.elem.context import lheight as ak_lheight
from akashi_core.elem.context import lcenter as ak_lcenter
from akashi_core.color import Color as ColorEnum
from akashi_core.color import color_value

if tp.TYPE_CHECKING:
    from akashi_core.elem.context import KronContext
    from .media import _BaseMediaField


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


@dataclass
class UnitField(_BaseTraitField):
    layer_indices: list[int] = field(default_factory=list, init=False)
    fb_size: tuple[int, int] = field(default=(0, 0), init=False)
    bg_color: str = "#00000000"  # transparent
    _start: sec = field(default_factory=lambda: sec(0))
    _end: sec = field(default_factory=lambda: sec(-1))
    _span_cnt: int | None = None
    _span_dur: sec | None = None


@dataclass
class UnitTrait(_BaseTrait):

    _name: str = "unit"

    def bg_color(self, color: tp.Union[str, 'ColorEnum']) -> tp.Self:
        self._priv.get_trait_field(self).bg_color = color_value(color)
        return self

    def fb_size(self, width: int, height: int, copy_to_layer_size: bool = True) -> tp.Self:
        new_fb_size = (width, height)
        self._priv.get_trait_field(self).fb_size = new_fb_size
        if copy_to_layer_size:
            # [XXX] Any ways to avoid directly accessing the other trait's field?
            transform_field = self._priv._get_transform_field("DO NOT CALL THIS! THIS IS FOR PRIVATE USAGE ONLY!")
            transform_field.layer_size = new_fb_size
        return self

    def range(self, start: sec | float, end: sec | float = -1) -> tp.Self:
        self._priv.get_trait_field(self)._start = sec(start)
        self._priv.get_trait_field(self)._end = sec(end)
        return self

    def span_cnt(self, count: int) -> tp.Self:
        self._priv.get_trait_field(self)._span_cnt = count
        self._priv.get_trait_field(self)._span_dur = None
        return self


def _apply_time_layout(child_layer: 'LayerField') -> sec:

    # AtomHandle case
    if not isinstance(child_layer._duration, sec):
        raise Exception('Passing a root handle to duration is prohibited within a frame')

    if child_layer.slice_offset == NOT_FIXED_SEC:
        child_layer.slice_offset = child_layer.frame_offset

    return child_layer.slice_offset + tp.cast(sec, child_layer._duration)


def _get_media_field(cur_ctx: 'KronContext', unit_layer: LayerField) -> '_BaseMediaField' | None:
    if unit_layer.t_video != -1:
        return cur_ctx.t_videos[unit_layer.t_video]
    elif unit_layer.t_audio != -1:
        return cur_ctx.t_audios[unit_layer.t_audio]
    return None


def _apply_slice(cur_ctx: 'KronContext', unit_layer: LayerField, unit_field: UnitField, max_to: sec) -> sec:

    if unit_field._start == sec(0) and unit_field._end == sec(-1):
        unit_layer._duration = max_to
        return max_to

    unit_start = unit_field._start
    unit_end = max_to if unit_field._end == -1 else unit_field._end
    unit_dur = unit_end - unit_start
    living_layer_indices: list[int] = []
    new_max_to: sec = sec(0)

    for layer_idx in unit_field.layer_indices:
        cur_layer = cur_ctx.layers[layer_idx]

        layer_from = cur_layer.frame_offset if cur_layer.slice_offset == NOT_FIXED_SEC else cur_layer.slice_offset
        layer_to = layer_from + tp.cast(sec, cur_layer._duration)
        if layer_from <= unit_end and layer_to >= unit_start:

            old_slice_offset: sec = cur_layer.slice_offset
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

            new_layer_to = cur_layer.slice_offset + tp.cast(sec, cur_layer._duration)
            if new_layer_to > new_max_to:
                new_max_to = new_layer_to

            media_field = _get_media_field(cur_ctx, cur_layer)
            if cur_layer.layer_local_offset > sec(0) and media_field:
                media_slice_dur = media_field.end - media_field.start
                media_slice_part = int(cur_layer.layer_local_offset / media_slice_dur)
                cur_layer.layer_local_offset = cur_layer.layer_local_offset - (media_slice_part * media_slice_dur)

            if cur_layer.t_unit != -1:
                cur_layer_unit_field = cur_ctx.t_units[cur_layer.t_unit]
                cur_layer_unit_field._start = unit_start
                cur_layer_unit_field._end = unit_end
                cur_layer_new_to = _apply_slice(cur_ctx, cur_layer, cur_layer_unit_field, sec(0))
                cur_layer._duration = cur_layer_new_to - cur_layer.slice_offset

            living_layer_indices.append(layer_idx)
        else:
            cur_layer.defunct = True

    unit_field.layer_indices = living_layer_indices
    unit_layer._duration = unit_dur

    return new_max_to


def _apply_span(cur_ctx: 'KronContext', unit_layer: LayerField, unit_field: UnitField):

    def _duplicate_children(
            child_layer_indices: list[int], incr_dur: sec, append_layer_indices: bool) -> list[int]:

        new_child_layer_indices = []

        for layer_idx in child_layer_indices:
            cur_layer = cur_ctx.layers[layer_idx]
            if cur_layer.defunct:
                continue

            dup_layer = copy.deepcopy(cur_layer)
            dup_layer.slice_offset += acc_duration
            dup_layer.frame_offset += acc_duration

            if cur_layer.t_unit != -1:
                unit_field = copy.deepcopy(cur_ctx.t_units[cur_layer.t_unit])
                gctx.get_ctx().t_units.append(unit_field)
                new_unit_trait_idx = len(gctx.get_ctx().t_units) - 1
                dup_layer.t_unit = new_unit_trait_idx

            new_child_layer_indices.append(
                register_layer_field(dup_layer, dup_layer.key + f'__span_cnt_{cnt+1}', append_layer_indices)
            )

            if dup_layer.t_unit != -1:
                _dup_layer_unit_field = cur_ctx.t_units[dup_layer.t_unit]
                _dup_layer_unit_field.layer_indices = _duplicate_children(
                    _dup_layer_unit_field.layer_indices, incr_dur, False)

        return new_child_layer_indices

    if unit_field._span_cnt and unit_field._span_cnt > 1:
        org_duration: sec = tp.cast(sec, unit_layer._duration)
        acc_duration: sec = org_duration
        child_layer_indices = copy.deepcopy(unit_field.layer_indices)
        for cnt in range(unit_field._span_cnt - 1):
            _duplicate_children(child_layer_indices, acc_duration, True)
            acc_duration += org_duration

        unit_layer._duration = tp.cast(sec, unit_layer._duration) * unit_field._span_cnt

    elif unit_field._span_dur:
        raise NotImplementedError()


@dataclass
class SpatialFrameGuard:

    _layer_idx: int

    def enter(self):
        gctx.get_ctx()._cur_unit_ids.append(self._layer_idx)

    def exit(self):
        cur_ctx = gctx.get_ctx()
        cur_unit_layer = cur_ctx.layers[cur_ctx._cur_unit_ids[-1]]
        assert cur_unit_layer.t_unit != -1
        cur_unit_field = cur_ctx.t_units[cur_unit_layer.t_unit]

        if not isinstance(cur_unit_layer._duration, sec):
            raise Exception('Invalid duration found')

        # Apply time/space layouts to its children
        max_to: sec = sec(0)
        for layout_idx, layer_idx in enumerate(cur_unit_field.layer_indices):
            cur_layer = cur_ctx.layers[layer_idx]
            # _apply_space_layout(cur_unit_layer, cur_layer, layout_idx)
            if (layer_to := _apply_time_layout(cur_layer)) and layer_to > max_to:
                max_to = layer_to

        # Apply slice to its children and fix the duration
        _apply_slice(cur_ctx, cur_unit_layer, cur_unit_field, max_to)

        # Span the slice and fix the duration
        _apply_span(cur_ctx, cur_unit_layer, cur_unit_field)

        # Pop this frame
        cur_ctx._cur_unit_ids.pop()


@dataclass
class TimelineFrameGuard:

    _layer_idx: int

    def enter(self):
        gctx.get_ctx()._cur_unit_ids.append(self._layer_idx)
        return self

    def exit(self):

        cur_ctx = gctx.get_ctx()
        cur_unit_layer = cur_ctx.layers[cur_ctx._cur_unit_ids[-1]]
        assert cur_unit_layer.t_unit != -1
        cur_unit_field = cur_ctx.t_units[cur_unit_layer.t_unit]

        if not isinstance(cur_unit_layer._duration, sec):
            raise Exception('Invalid duration found')

        acc_duration: sec = sec(0)
        for layer_idx in cur_unit_field.layer_indices:

            cur_layer = cur_ctx.layers[layer_idx]
            if cur_layer.slice_offset == NOT_FIXED_SEC:
                cur_layer.slice_offset = cur_layer.frame_offset
            if cur_layer.t_unit != -1:
                cur_layer_unit_field = cur_ctx.t_units[cur_layer.t_unit]
                self._update_unit_layer_slice_offset(cur_layer, cur_layer_unit_field, acc_duration)
            else:
                cur_layer.slice_offset += acc_duration

            if not isinstance(cur_layer._duration, sec):
                raise Exception('Passing a layer handle to duration is prohibited for child layers of an scene layer')

            acc_duration += cur_layer._duration

        cur_unit_layer._duration = acc_duration

        max_to = (cur_unit_layer.frame_offset if cur_unit_layer.slice_offset ==
                  NOT_FIXED_SEC else cur_unit_layer.slice_offset) + cur_unit_layer._duration
        # Apply slice to its children and fix the duration
        _apply_slice(cur_ctx, cur_unit_layer, cur_unit_field, max_to)

        # Span the slice and fix the duration
        _apply_span(cur_ctx, cur_unit_layer, cur_unit_field)

        cur_ctx._cur_unit_ids.pop()

    @staticmethod
    def _update_unit_layer_slice_offset(unit_entry: LayerField, unit_field: UnitField, new_offset: sec):

        cur_ctx = gctx.get_ctx()
        unit_entry.slice_offset += new_offset
        for layer_idx in unit_field.layer_indices:
            cur_layer = cur_ctx.layers[layer_idx]
            if cur_layer.t_unit != -1:
                cur_layer_unit_field = cur_ctx.t_units[cur_layer.t_unit]
                TimelineFrameGuard._update_unit_layer_slice_offset(cur_layer, cur_layer_unit_field, new_offset)
            else:
                cur_layer.slice_offset += new_offset
