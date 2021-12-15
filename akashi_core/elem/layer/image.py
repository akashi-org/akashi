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

if tp.TYPE_CHECKING:
    from akashi_core.elem.atom import AtomHandle


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
