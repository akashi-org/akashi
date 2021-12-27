# pyright: reportPrivateUsage=false
from __future__ import annotations
from dataclasses import dataclass
import typing as tp

from .base import (
    FittableDurationTrait,
    PositionField,
    PositionTrait,
    ShaderField,
    LayerField,
    LayerTrait
)
from .base import peek_entry, register_entry, frag, poly
from akashi_core.pysl import _gl as gl
from akashi_core.pysl.shader import ShaderCompiler, _frag_shader_header, _poly_shader_header
from akashi_core.pysl.shader import EntryFragFn, EntryPolyFn
from akashi_core.pysl.shader import _NamedEntryFragFn, _NamedEntryPolyFn, _TEntryFnOpaque


@dataclass
class ImageUniform:
    texture_arr: tp.Final[gl.uniform[gl.sampler2DArray]] = gl._uniform_default()


@dataclass
class ImageFragBuffer(frag, ImageUniform, gl._LayerFragInput):
    ...


@dataclass
class ImagePolyBuffer(poly, ImageUniform, gl._LayerPolyOutput):
    ...


_ImageFragFn = EntryFragFn[ImageFragBuffer] | _TEntryFnOpaque[_NamedEntryFragFn[ImageFragBuffer]]
_ImagePolyFn = EntryPolyFn[ImagePolyBuffer] | _TEntryFnOpaque[_NamedEntryPolyFn[ImagePolyBuffer]]


@dataclass
class ImageLocalField:
    srcs: list[str]
    stretch: bool = False


@dataclass
class ImageEntry(ShaderField, PositionField, LayerField, ImageLocalField):
    ...


@dataclass
class ImageHandle(FittableDurationTrait, PositionTrait, LayerTrait):

    def frag(self, *frag_fns: _ImageFragFn, preamble: tuple[str, ...] = tuple()) -> 'ImageHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, ImageEntry):
            cur_layer.frag_shader = ShaderCompiler(frag_fns, ImageFragBuffer, _frag_shader_header, preamble)
        return self

    def poly(self, *poly_fns: _ImagePolyFn, preamble: tuple[str, ...] = tuple()) -> 'ImageHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, ImageEntry):
            cur_layer.poly_shader = ShaderCompiler(poly_fns, ImagePolyBuffer, _poly_shader_header, preamble)
        return self

    def stretch(self, stretch: bool) -> 'ImageHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, ImageEntry):
            cur_layer.stretch = stretch
        return self


class image(object):

    frag: tp.ClassVar[tp.Type[ImageFragBuffer]] = ImageFragBuffer
    poly: tp.ClassVar[tp.Type[ImagePolyBuffer]] = ImagePolyBuffer

    def __new__(cls, src: str | list[str], key: str = '') -> ImageHandle:

        entry = ImageEntry([src] if isinstance(src, str) else src)
        idx = register_entry(entry, 'IMAGE', key)
        return ImageHandle(idx)
