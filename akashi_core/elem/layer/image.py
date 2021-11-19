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

    __idx: int

    def pos(self, x: int, y: int):
        if (cur_layer := peek_entry(self.__idx)) and isinstance(cur_layer, ImageEntry):
            cur_layer.pos = (x, y)
        return self

    def duration(self, duration: sec):
        if (cur_layer := peek_entry(self.__idx)) and isinstance(cur_layer, ImageEntry):
            cur_layer.duration = duration
        return self

    def offset(self, offset: sec):
        if (cur_layer := peek_entry(self.__idx)) and isinstance(cur_layer, ImageEntry):
            cur_layer.atom_offset = offset
        return self

    def frag(self, frag_shader: FragShader):
        if (cur_layer := peek_entry(self.__idx)) and isinstance(cur_layer, ImageEntry):
            cur_layer.frag_shader = frag_shader
        return self

    def poly(self, poly_shader: PolygonShader):
        if (cur_layer := peek_entry(self.__idx)) and isinstance(cur_layer, ImageEntry):
            cur_layer.poly_shader = poly_shader
        return self


def image(src: str, key: str = '') -> ImageHandle:

    entry = ImageEntry(src)
    idx = register_entry(entry, 'IMAGE', key)
    return ImageHandle(idx)
