from __future__ import annotations
from dataclasses import dataclass
import typing as tp
from abc import abstractmethod, ABCMeta

from akashi_core.elem.context import _GlobalKronContext as gctx
from akashi_core.time import sec
from akashi_core.pysl import FragShader, PolygonShader

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


@tp.final
@dataclass
class ImageEntry(ShaderField, PositionField, LayerField, ImageLocalField):
    ...


@tp.final
@dataclass
class ImageHandle(FittableDurationTrait, ShaderTrait, PositionTrait, LayerTrait):

    def stretch(self, stretch: bool):
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, ImageEntry):
            cur_layer.stretch = stretch
        return self


def image(srcs: list[str], key: str = '') -> ImageHandle:

    entry = ImageEntry(srcs)
    idx = register_entry(entry, 'IMAGE', key)
    return ImageHandle(idx)
