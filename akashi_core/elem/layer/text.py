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


@tp.final
@dataclass
class TextStyle:
    font_size: int = 30
    font_path: str = ""
    fill: str = ""  # rrggbb


@dataclass
class TextLocalField:
    text: str
    style: TextStyle = TextStyle()


@tp.final
@dataclass
class TextEntry(ShaderField, DurationField, PositionField, LayerField, TextLocalField):
    ...


@tp.final
@dataclass
class TextHandle(ShaderTrait, DurationTrait, PositionTrait, LayerTrait):

    def font_size(self, size: int):
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, TextEntry):
            cur_layer.style.font_size = size
        return self

    def font_path(self, path: str):
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, TextEntry):
            cur_layer.style.font_path = path
        return self

    def font_fill(self, color: str):
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, TextEntry):
            cur_layer.style.fill = color
        return self


def text(text: str, key: str = '') -> TextHandle:

    entry = TextEntry(text)
    idx = register_entry(entry, 'TEXT', key)
    return TextHandle(idx)
