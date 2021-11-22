from dataclasses import dataclass
import typing as tp
from abc import abstractmethod, ABCMeta

from akashi_core.elem.context import _GlobalKronContext as gctx
from akashi_core.time import sec
from akashi_core.pysl import FragShader, PolygonShader

from .base import (
    ShaderField,
    ShaderTrait,
    LayerField,
    LayerTrait
)
from .base import peek_entry, register_entry


@tp.final
@dataclass
class EffectEntry(ShaderField, LayerField):
    ...


@tp.final
@dataclass
class EffectHandle(ShaderTrait, LayerTrait):
    ...


def effect(key: str = '') -> EffectHandle:

    entry = EffectEntry()
    idx = register_entry(entry, 'EFFECT', key)
    return EffectHandle(idx)
