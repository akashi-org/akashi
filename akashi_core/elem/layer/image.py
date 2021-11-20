from __future__ import annotations
from dataclasses import dataclass
import typing as tp
from abc import abstractmethod, ABCMeta

from akashi_core.elem.context import _GlobalKronContext as gctx
from akashi_core.time import sec
from akashi_core.pysl import FragShader, PolygonShader

from .base import (
    PositionField,
    PositionTrait,
    DurationField,
    DurationTrait,
    ShaderField,
    ShaderTrait,
    LayerField,
    LayerTrait
)
from .base import peek_entry, register_entry


@dataclass
class ImageLocalField:
    src: str


@tp.final
@dataclass
class ImageEntry(ShaderField, DurationField, PositionField, LayerField, ImageLocalField):
    ...


@tp.final
@dataclass
class ImageHandle(ShaderTrait, DurationTrait, PositionTrait, LayerTrait):
    ...


def image(src: str, key: str = '') -> ImageHandle:

    entry = ImageEntry(src)
    idx = register_entry(entry, 'IMAGE', key)
    return ImageHandle(idx)
