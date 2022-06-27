# pyright: reportPrivateUsage=false
from __future__ import annotations
from dataclasses import dataclass, field
import typing as tp
from typing import runtime_checkable

from akashi_core.time import sec
from akashi_core.elem.context import lcenter

from .base import (
    MediaField,
    MediaTrait,
    TransformField,
    TransformTrait,
    TextureField,
    TextureTrait,
    LayerField,
    LayerTrait,
    ShaderField,
    _calc_media_duration
)
from .base import peek_entry, register_entry, frag, poly, LayerRef

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
    ...


@runtime_checkable
class HasVideoLocalField(tp.Protocol):
    video: VideoLocalField


@dataclass
class RequiredParams:
    _req_src: str


@dataclass
class VideoLocalTrait:
    _idx: int


@dataclass
class VideoEntry(LayerField, RequiredParams):

    video: VideoLocalField = field(init=False)
    media: MediaField = field(init=False)
    transform: TransformField = field(init=False)
    tex: TextureField = field(init=False)
    shader: ShaderField = field(init=False)

    def __post_init__(self):
        self._duration = sec(-1)
        self.video = VideoLocalField()
        self.media = MediaField(self._req_src)
        self.transform = TransformField()
        self.tex = TextureField()
        self.shader = ShaderField()


@dataclass
class VideoTrait(LayerTrait):

    video: VideoLocalTrait = field(init=False)
    media: MediaTrait = field(init=False)
    transform: TransformTrait = field(init=False)
    tex: TextureTrait = field(init=False)

    def __post_init__(self):
        self.video = VideoLocalTrait(self._idx)
        self.media = MediaTrait(self._idx)
        self.tex = TextureTrait(self._idx)
        self.transform = TransformTrait(self._idx)

    def frag(self, *frag_fns: _VideoFragFn, preamble: tuple[str, ...] = tuple()) -> 'VideoTrait':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, VideoEntry):
            cur_layer.shader.frag_shader = ShaderCompiler(
                frag_fns, VideoFragBuffer, _video_frag_shader_header, preamble)
        return self

    def poly(self, *poly_fns: _VideoPolyFn, preamble: tuple[str, ...] = tuple()) -> 'VideoTrait':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, VideoEntry):
            cur_layer.shader.poly_shader = ShaderCompiler(
                poly_fns, VideoPolyBuffer, _video_poly_shader_header, preamble)
        return self


video_frag = VideoFragBuffer

video_poly = VideoPolyBuffer

VideoTraitFn = tp.Callable[[VideoTrait], tp.Any]


def video(src: str, *trait_fns: VideoTraitFn) -> LayerRef:

    entry = VideoEntry(src)
    idx = register_entry(entry, 'VIDEO', '')
    t = VideoTrait(idx)
    t.transform.pos(*lcenter())
    [tfn(t) for tfn in trait_fns]

    entry._duration = _calc_media_duration(entry.media)
    return LayerRef(idx)
