# pyright: reportPrivateUsage=false
from dataclasses import dataclass
import typing as tp

from .base import (
    FittableDurationTrait,
    ShaderField,
    ShaderTrait,
    LayerField,
    LayerTrait
)
from .base import register_entry


@dataclass
class EffectEntry(ShaderField, LayerField):
    ...


@dataclass
class EffectHandle(FittableDurationTrait, ShaderTrait, LayerTrait):
    ...


def effect(key: str = '') -> EffectHandle:

    entry = EffectEntry()
    idx = register_entry(entry, 'EFFECT', key)
    return EffectHandle(idx)
