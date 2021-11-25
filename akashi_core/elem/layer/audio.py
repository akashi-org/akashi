from __future__ import annotations
from dataclasses import dataclass
import typing as tp
from abc import abstractmethod, ABCMeta

from akashi_core.elem.context import _GlobalKronContext as gctx
from akashi_core.time import sec

from .base import LayerField, LayerTrait
from .base import peek_entry, register_entry


@dataclass
class AudioLocalField:
    src: str
    gain: float = 1.0


@tp.final
@dataclass
class AudioEntry(LayerField, AudioLocalField):
    ...


@tp.final
@dataclass
class AudioHandle(LayerTrait):

    def duration(self, duration: sec) -> 'AudioHandle':
        return super().duration(duration)

    def gain(self, gain: float) -> 'AudioHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, AudioEntry):
            cur_layer.gain = gain
        return self


def audio(src: str, key: str = '') -> AudioHandle:

    entry = AudioEntry(src)
    idx = register_entry(entry, 'AUDIO', key)
    return AudioHandle(idx)
