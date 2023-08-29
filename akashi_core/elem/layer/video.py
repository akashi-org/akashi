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
    ShaderTrait,
    _calc_media_duration
)
from .base import peek_entry, register_entry, LayerRef


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
    shader: ShaderTrait = field(init=False)

    def __post_init__(self):
        self.video = VideoLocalTrait(self._idx)
        self.media = MediaTrait(self._idx)
        self.transform = TransformTrait(self._idx)
        self.tex = TextureTrait(self._idx)
        self.shader = ShaderTrait(self._idx)


VideoTraitFn = tp.Callable[[VideoTrait], tp.Any]


def video(src: str, *trait_fns: VideoTraitFn) -> LayerRef:

    entry = VideoEntry(src)
    idx = register_entry(entry, 'VIDEO', '')
    t = VideoTrait(idx)
    t.transform.pos(*lcenter())
    [tfn(t) for tfn in trait_fns]

    entry._duration = _calc_media_duration(entry.media)
    return LayerRef(idx)
