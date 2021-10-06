from __future__ import annotations
from dataclasses import dataclass
import typing as tp
from abc import abstractmethod, ABCMeta

from akashi_core.elem.context import _GlobalKronContext as gctx
from akashi_core.time import sec
from akashi_core.pysl import VideoFragShader, VideoGeomShader

from .base import PositionField, PositionTrait, LayerField, LayerTrait
from .base import peek_entry, register_entry

# temporary
from .base import DurationField, DurationTrait


@dataclass
class VideoLocalField:
    src: str
    frame: tuple[int, int] = (0, -1)
    gain: float = 1.0
    atom_offset: sec = sec(0)
    frag_shader: tp.Optional[VideoFragShader] = None
    geom_shader: tp.Optional[VideoGeomShader] = None


# [TODO] remove DurationConcept later?


@tp.final
@dataclass
class VideoEntry(DurationField, PositionField, LayerField, VideoLocalField):
    ...


@tp.final
@dataclass
class VideoHandle(DurationTrait, PositionTrait, LayerTrait):

    __idx: int

    def pos(self, x: int, y: int):
        if (cur_layer := peek_entry(self.__idx)) and isinstance(cur_layer, VideoEntry):
            cur_layer.pos = (x, y)
        return self

    def duration(self, duration: sec):
        if (cur_layer := peek_entry(self.__idx)) and isinstance(cur_layer, VideoEntry):
            cur_layer.duration = duration
        return self

    def frame(self, begin_frame: int, end_frame: int = -1):
        if (cur_layer := peek_entry(self.__idx)) and isinstance(cur_layer, VideoEntry):
            cur_layer.frame = (begin_frame, end_frame)
        return self

    def gain(self, gain: float):
        if (cur_layer := peek_entry(self.__idx)) and isinstance(cur_layer, VideoEntry):
            cur_layer.gain = gain
        return self

    def offset(self, offset: sec):
        if (cur_layer := peek_entry(self.__idx)) and isinstance(cur_layer, VideoEntry):
            cur_layer.atom_offset = offset
        return self

    def frag(self, frag_shader: VideoFragShader):
        if (cur_layer := peek_entry(self.__idx)) and isinstance(cur_layer, VideoEntry):
            cur_layer.frag_shader = frag_shader
        return self

    def geom(self, geom_shader: VideoGeomShader):
        if (cur_layer := peek_entry(self.__idx)) and isinstance(cur_layer, VideoEntry):
            cur_layer.geom_shader = geom_shader
        return self


def video(src: str, key: str = '') -> VideoHandle:

    entry = VideoEntry(src)
    idx = register_entry(entry, 'VIDEO', key)
    return VideoHandle(idx)
