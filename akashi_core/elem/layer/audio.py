# pyright: reportPrivateUsage=false
from __future__ import annotations
from dataclasses import dataclass
import typing as tp

from akashi_core.time import sec
from .base import LayerField, LayerTrait
from .base import peek_entry, register_entry


@dataclass
class AudioLocalField:
    src: str
    gain: float = 1.0
    start: sec = sec(0)


@dataclass
class AudioEntry(LayerField, AudioLocalField):

    def __post_init__(self):
        self.duration = sec(-1)


@dataclass
class AudioHandle(LayerTrait):

    def gain(self, gain: float) -> 'AudioHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, AudioEntry):
            cur_layer.gain = gain
        return self

    def start(self, start: sec) -> 'AudioHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, AudioEntry):
            cur_layer.start = start
        return self


class audio(object):

    def __new__(cls, src: str, key: str = '') -> AudioHandle:

        entry = AudioEntry(src)
        idx = register_entry(entry, 'AUDIO', key)
        return AudioHandle(idx)
