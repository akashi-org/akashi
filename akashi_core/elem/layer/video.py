# pyright: reportPrivateUsage=false
from __future__ import annotations
from dataclasses import dataclass
import typing as tp

from akashi_core.time import sec

from .base import PositionField, PositionTrait, LayerField, LayerTrait
from .base import peek_entry, register_entry, frag, poly

from akashi_core.pysl import _gl as gl
from akashi_core.pysl.shader import ShaderCompiler, _video_frag_shader_header, _video_poly_shader_header
from akashi_core.pysl.shader import LEntryFragFn, LEntryPolyFn
from akashi_core.pysl.shader import _NamedEntryFragFn, _NamedEntryPolyFn, _TEntryFnOpaque


@dataclass
class GS_OUT_V:
    vLumaUvs: gl.vec2 = gl._default()
    vChromaUvs: gl.vec2 = gl._default()


@dataclass
class VideoFragInput:
    fs_in: tp.Final['gl.in_t'[GS_OUT_V]] = gl._in_t_default()


@dataclass
class VS_OUT_V:
    vLumaUvs: gl.vec2 = gl._default()
    vChromaUvs: gl.vec2 = gl._default()


@dataclass
class VideoPolyOutput:
    vs_out: 'gl.out_t'[VS_OUT_V] = gl._out_t_default()


@dataclass
class VideoUniform:
    textureY: tp.Final[gl.uniform[gl.sampler2D]] = gl._uniform_default()
    textureCb: tp.Final[gl.uniform[gl.sampler2D]] = gl._uniform_default()
    textureCr: tp.Final[gl.uniform[gl.sampler2D]] = gl._uniform_default()


@dataclass
class VideoFragBuffer(frag, VideoUniform, VideoFragInput):
    ...


@dataclass
class VideoPolyBuffer(poly, VideoUniform, VideoPolyOutput):
    ...


_VideoFragFn = LEntryFragFn[VideoFragBuffer] | _TEntryFnOpaque[_NamedEntryFragFn[VideoFragBuffer]]
_VideoPolyFn = LEntryPolyFn[VideoPolyBuffer] | _TEntryFnOpaque[_NamedEntryPolyFn[VideoPolyBuffer]]


@dataclass
class VideoLocalField:
    src: str
    frame: tuple[int, int] = (0, -1)
    gain: float = 1.0
    stretch: bool = False
    start: sec = sec(0)  # temporary
    atom_offset: sec = sec(0)
    frag_shader: tp.Optional[ShaderCompiler] = None
    poly_shader: tp.Optional[ShaderCompiler] = None


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

    def frag(self, *frag_fns: _VideoFragFn, preamble: tuple[str, ...] = tuple()) -> 'VideoHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, VideoEntry):
            cur_layer.frag_shader = ShaderCompiler(frag_fns, VideoFragBuffer, _video_frag_shader_header, preamble)
        return self

    def poly(self, *poly_fns: _VideoPolyFn, preamble: tuple[str, ...] = tuple()) -> 'VideoHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, VideoEntry):
            cur_layer.poly_shader = ShaderCompiler(poly_fns, VideoPolyBuffer, _video_poly_shader_header, preamble)
        return self


class video(object):

    frag: tp.ClassVar[tp.Type[VideoFragBuffer]] = VideoFragBuffer
    poly: tp.ClassVar[tp.Type[VideoPolyBuffer]] = VideoPolyBuffer

    def __new__(cls, src: str, key: str = '') -> VideoHandle:

        entry = VideoEntry(src)
        idx = register_entry(entry, 'VIDEO', key)
        return VideoHandle(idx)
