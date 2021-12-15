# pyright: reportPrivateUsage=false
from __future__ import annotations
from dataclasses import dataclass
import typing as tp
from typing import overload
from abc import abstractmethod, ABCMeta

from akashi_core.elem.context import _GlobalKronContext as gctx
from akashi_core.time import sec
from akashi_core.pysl import VideoFragShader, VideoPolygonShader
from akashi_core.pysl.shader import EntryFragFn, EntryPolyFn

from .base import PositionField, PositionTrait, LayerField, LayerTrait
from .base import peek_entry, register_entry


@dataclass
class VideoLocalField:
    src: str
    frame: tuple[int, int] = (0, -1)
    gain: float = 1.0
    stretch: bool = False
    start: sec = sec(0)  # temporary
    atom_offset: sec = sec(0)
    frag_shader: tp.Optional[VideoFragShader] = None
    poly_shader: tp.Optional[VideoPolygonShader] = None


# [TODO] remove DurationConcept later?


@dataclass
class VideoEntry(PositionField, LayerField, VideoLocalField):
    ...


@dataclass
class VideoHandle(PositionTrait, LayerTrait):

    def frame(self, begin_frame: int, end_frame: int = -1) -> 'VideoHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, VideoEntry):
            cur_layer.frame = (begin_frame, end_frame)
        return self

    def gain(self, gain: float) -> 'VideoHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, VideoEntry):
            cur_layer.gain = gain
        return self

    def stretch(self, stretch: bool) -> 'VideoHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, VideoEntry):
            cur_layer.stretch = stretch
        return self

    def start(self, start: sec) -> 'VideoHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, VideoEntry):
            cur_layer.start = start
        return self

    def offset(self, offset: sec) -> 'VideoHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, VideoEntry):
            cur_layer.atom_offset = offset
        return self

    @overload
    def frag(self, *frag_shaders: EntryFragFn) -> 'VideoHandle':
        ...

    @overload
    def frag(self, *frag_shaders: VideoFragShader) -> 'VideoHandle':
        ...

    def frag(self, *frag_shaders: tp.Any) -> 'VideoHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, VideoEntry):
            if isinstance(frag_shaders[0], VideoFragShader):
                cur_layer.frag_shader = frag_shaders[0]
            elif callable(frag_shaders[0]):
                cur_layer.frag_shader = VideoFragShader()
                cur_layer.frag_shader._inline_shaders = frag_shaders
        return self

    @overload
    def poly(self, *poly_shaders: EntryPolyFn) -> 'VideoHandle':
        ...

    @overload
    def poly(self, *poly_shaders: VideoPolygonShader) -> 'VideoHandle':
        ...

    def poly(self, *poly_shaders: tp.Any) -> 'VideoHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, VideoEntry):
            if isinstance(poly_shaders[0], VideoPolygonShader):
                cur_layer.poly_shader = poly_shaders[0]
            elif callable(poly_shaders[0]):
                cur_layer.poly_shader = VideoPolygonShader()
                cur_layer.poly_shader._inline_shaders = poly_shaders
        return self


def video(src: str, key: str = '') -> VideoHandle:

    entry = VideoEntry(src)
    idx = register_entry(entry, 'VIDEO', key)
    return VideoHandle(idx)
