# pyright: reportPrivateUsage=false
from __future__ import annotations
from dataclasses import dataclass, field
import typing as tp
from typing import runtime_checkable

from akashi_core.time import sec
from .base import LayerField, LayerTrait
from .base import peek_entry, register_entry

from .base import (
    MediaField,
    MediaTrait
)


@dataclass
class AudioLocalField:
    ...


@runtime_checkable
class HasAudioLocalField(tp.Protocol):
    audio: AudioLocalField


@dataclass
class RequiredParams:
    _req_src: str


@dataclass
class AudioLocalTrait:
    _idx: int


@dataclass
class AudioEntry(LayerField, RequiredParams):

    audio: AudioLocalField = field(init=False)
    media: MediaField = field(init=False)

    def __post_init__(self):
        self.audio = AudioLocalField()
        self.media = MediaField(src=self._req_src)
        self.duration = sec(-1)


@dataclass
class AudioHandle(LayerTrait):

    audio: AudioLocalTrait = field(init=False)
    media: MediaTrait = field(init=False)

    def __post_init__(self):
        self.audio = AudioLocalTrait(self._idx)
        self.media = MediaTrait(self._idx)


class audio(object):

    def __new__(cls, src: str, key: str = '') -> AudioHandle:

        entry = AudioEntry(src)
        idx = register_entry(entry, 'AUDIO', key)
        return AudioHandle(idx)
