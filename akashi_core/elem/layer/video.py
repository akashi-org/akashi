# pyright: reportPrivateUsage=false
from __future__ import annotations
from dataclasses import dataclass
import typing as tp

from akashi_core.time import sec
from akashi_core.pysl import VideoFragShader, VideoPolygonShader

from .base import PositionField, PositionTrait, LayerField, LayerTrait
from .base import peek_entry, register_entry

if tp.TYPE_CHECKING:
    from akashi_core.pysl.shader import GEntryFragFn, GEntryPolyFn


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

    def frag(self, *frag_shaders: 'GEntryFragFn') -> 'VideoHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, VideoEntry):
            cur_layer.frag_shader = VideoFragShader(frag_shaders)
        return self

    def poly(self, *poly_shaders: 'GEntryPolyFn') -> 'VideoHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, VideoEntry):
            cur_layer.poly_shader = VideoPolygonShader(poly_shaders)
        return self


def video(src: str, key: str = '') -> VideoHandle:

    entry = VideoEntry(src)
    idx = register_entry(entry, 'VIDEO', key)
    return VideoHandle(idx)
