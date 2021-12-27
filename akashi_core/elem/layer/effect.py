# pyright: reportPrivateUsage=false
from dataclasses import dataclass
import typing as tp

from .base import (
    FittableDurationTrait,
    ShaderField,
    LayerField,
    LayerTrait
)
from .base import register_entry, peek_entry, frag, poly
from akashi_core.pysl import _gl as gl
from akashi_core.pysl.shader import ShaderCompiler, _frag_shader_header, _poly_shader_header
from akashi_core.pysl.shader import EntryFragFn, EntryPolyFn
from akashi_core.pysl.shader import _NamedEntryFragFn, _NamedEntryPolyFn, _TEntryFnOpaque


@dataclass
class EffectUniform:
    texture0: tp.Final[gl.uniform[gl.sampler2D]] = gl._uniform_default()


@dataclass
class EffectFragBuffer(frag, EffectUniform, gl._LayerFragInput):
    ...


@dataclass
class EffectPolyBuffer(poly, EffectUniform, gl._LayerPolyOutput):
    ...


@dataclass
class EffectEntry(ShaderField, LayerField):
    ...


_EffectFragFn = EntryFragFn[EffectFragBuffer] | _TEntryFnOpaque[_NamedEntryFragFn[EffectFragBuffer]]
_EffectPolyFn = EntryPolyFn[EffectPolyBuffer] | _TEntryFnOpaque[_NamedEntryPolyFn[EffectPolyBuffer]]


@dataclass
class EffectHandle(FittableDurationTrait, LayerTrait):

    def frag(self, *frag_fns: _EffectFragFn, preamble: tuple[str, ...] = tuple()) -> 'EffectHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, EffectEntry):
            cur_layer.frag_shader = ShaderCompiler(frag_fns, EffectFragBuffer, _frag_shader_header, preamble)
        return self

    def poly(self, *poly_fns: _EffectPolyFn, preamble: tuple[str, ...] = tuple()) -> 'EffectHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, EffectEntry):
            cur_layer.poly_shader = ShaderCompiler(poly_fns, EffectPolyBuffer, _poly_shader_header, preamble)
        return self


class effect(object):

    frag: tp.ClassVar[tp.Type[EffectFragBuffer]] = EffectFragBuffer
    poly: tp.ClassVar[tp.Type[EffectPolyBuffer]] = EffectPolyBuffer

    def __new__(cls, key: str = '') -> EffectHandle:
        entry = EffectEntry()
        idx = register_entry(entry, 'EFFECT', key)
        return EffectHandle(idx)
