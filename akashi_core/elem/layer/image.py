# pyright: reportPrivateUsage=false
from __future__ import annotations
from dataclasses import dataclass
import typing as tp

from .base import (
    FittableDurationTrait,
    PositionField,
    PositionTrait,
    ShaderField,
    ShaderTrait,
    LayerField,
    LayerTrait
)
from .base import peek_entry, register_entry


@dataclass
class ImageLocalField:
    srcs: list[str]
    stretch: bool = False


@dataclass
class ImageEntry(ShaderField, PositionField, LayerField, ImageLocalField):
    ...


@dataclass
class ImageHandle(FittableDurationTrait, ShaderTrait, PositionTrait, LayerTrait):

    def stretch(self, stretch: bool) -> 'ImageHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, ImageEntry):
            cur_layer.stretch = stretch
        return self


def image(src: tp.Union[str, list[str]], key: str = '') -> ImageHandle:

    entry = ImageEntry([src] if isinstance(src, str) else src)
    idx = register_entry(entry, 'IMAGE', key)
    return ImageHandle(idx)
