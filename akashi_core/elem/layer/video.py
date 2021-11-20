from __future__ import annotations
from dataclasses import dataclass
import typing as tp
from abc import abstractmethod, ABCMeta

from akashi_core.elem.context import _GlobalKronContext as gctx
from akashi_core.time import sec
from akashi_core.pysl import VideoFragShader, PolygonShader

from .base import PositionField, PositionTrait, LayerField, LayerTrait
from .base import peek_entry, register_entry


@dataclass
class VideoLocalField:
    src: str
    frame: tuple[int, int] = (0, -1)
    gain: float = 1.0
    start: sec = sec(0)  # temporary
    atom_offset: sec = sec(0)
    frag_shader: tp.Optional[VideoFragShader] = None
    poly_shader: tp.Optional[PolygonShader] = None


# [TODO] remove DurationConcept later?


@tp.final
@dataclass
class VideoEntry(PositionField, LayerField, VideoLocalField):
    ...


@tp.final
@dataclass
class VideoHandle(PositionTrait, LayerTrait):

    def frame(self, begin_frame: int, end_frame: int = -1):
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, VideoEntry):
            cur_layer.frame = (begin_frame, end_frame)
        return self

    def gain(self, gain: float):
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, VideoEntry):
            cur_layer.gain = gain
        return self

    def start(self, start: sec):
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, VideoEntry):
            cur_layer.start = start
        return self

    def offset(self, offset: sec):
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, VideoEntry):
            cur_layer.atom_offset = offset
        return self

    def frag(self, frag_shader: VideoFragShader):
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, VideoEntry):
            cur_layer.frag_shader = frag_shader
        return self

    def poly(self, poly_shader: PolygonShader):
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, VideoEntry):
            cur_layer.poly_shader = poly_shader
        return self


def video(src: str, key: str = '') -> VideoHandle:

    entry = VideoEntry(src)
    idx = register_entry(entry, 'VIDEO', key)
    return VideoHandle(idx)
