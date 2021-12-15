from dataclasses import dataclass
import typing as tp
from abc import abstractmethod, ABCMeta

from akashi_core.elem.context import _GlobalKronContext as gctx
from akashi_core.time import sec
from akashi_core.pysl import FragShader, PolygonShader

from .base import (
    FittableDurationTrait,
    ShaderField,
    ShaderTrait,
    LayerField,
    LayerTrait
)
from .base import peek_entry, register_entry

if tp.TYPE_CHECKING:
    from akashi_core.elem.atom import AtomHandle


@dataclass
class EffectEntry(ShaderField, LayerField):
    ...


@dataclass
class EffectHandle(FittableDurationTrait, ShaderTrait, LayerTrait):

    def ap(self, *hs: tp.Callable[['EffectHandle'], 'EffectHandle']) -> 'EffectHandle':
        [h(self) for h in hs]
        return self

    def duration(self, duration: sec) -> 'EffectHandle':
        return super().duration(duration)

    def fit_to(self, handle: 'AtomHandle') -> 'EffectHandle':
        return super().fit_to(handle)


def effect(key: str = '') -> EffectHandle:

    entry = EffectEntry()
    idx = register_entry(entry, 'EFFECT', key)
    return EffectHandle(idx)
