from __future__ import annotations
from . import compile_mixed_shaders, CompilerConfig
from . import _gl

from abc import ABCMeta
import typing as tp
from dataclasses import dataclass, field

ShaderKind = tp.Literal['AnyShader', 'FragShader', 'PolygonShader', 'GeomShader']

EntryFragFn = tp.Callable[['FragShader', _gl.inout_p[_gl.vec4]], _gl.expr | None]
EntryPolyFn = tp.Callable[['PolygonShader', _gl.inout_p[_gl.vec3]], _gl.expr | None]


TEntryFn = tp.TypeVar('TEntryFn', 'EntryFragFn', 'EntryPolyFn')

_T = tp.TypeVar('_T')


class TEntryFnOpaque(tp.Generic[_T]):
    ...


TNarrowEntryFnOpaque = tp.TypeVar('TNarrowEntryFnOpaque', TEntryFnOpaque['EntryFragFn'], TEntryFnOpaque['EntryPolyFn'])

GEntryFragFn = EntryFragFn | TEntryFnOpaque['EntryFragFn']
GEntryPolyFn = EntryPolyFn | TEntryFnOpaque['EntryPolyFn']

EntryFragFnGP = tuple[GEntryFragFn, ...]
EntryPolyFnGP = tuple[GEntryPolyFn, ...]


@dataclass
class ShaderModule(metaclass=ABCMeta):
    __glsl_version__: tp.ClassVar[str] = '#version 420 core\n'
    __header__: tp.ClassVar[list[str]] = []
    __preamble__: tp.ClassVar[list[str]] = []

    __kind__: tp.ClassVar[ShaderKind] = 'AnyShader'

    _assemble_cache: tp.Optional[str] = field(default=None, init=False)

    def _header(self, config: CompilerConfig.Config) -> str:
        if not config['pretty_compile']:
            return ''.join(self.__header__)
        else:
            return '\n'.join(self.__header__) + '\n'

    def _preamble(self, config: CompilerConfig.Config) -> str:
        if len(self.__preamble__) == 0:
            return self.__glsl_version__
        else:
            if not config['pretty_compile']:
                return self.__glsl_version__ + '\n'.join(self.__preamble__) + '\n'
            else:
                return self.__glsl_version__ + '\n'.join(self.__preamble__) + '\n' + '\n'

    def _invalidate_cache(self):
        self._assemble_cache = None


@dataclass
class BasicUniform:

    time: _gl.uniform[float] = field(default=_gl.uniform.default(), init=False)
    global_time: _gl.uniform[float] = field(default=_gl.uniform.default(), init=False)
    local_duration: _gl.uniform[float] = field(default=_gl.uniform.default(), init=False)
    fps: _gl.uniform[float] = field(default=_gl.uniform.default(), init=False)
    resolution: _gl.uniform[_gl.vec2] = field(default=_gl.uniform.default(), init=False)


@dataclass
class AnyShader(ShaderModule):

    __kind__: tp.ClassVar[ShaderKind] = 'AnyShader'


@dataclass
class GS_OUT:
    vUvs: _gl.vec2
    sprite_idx: float


@dataclass
class FragShader(BasicUniform, ShaderModule):

    __kind__: tp.ClassVar[ShaderKind] = 'FragShader'

    __header__: tp.ClassVar[list[str]] = [
        'uniform float time;',
        'uniform float global_time;',
        'uniform float local_duration;',
        'uniform float fps;',
        'uniform vec2 resolution;',
        'uniform sampler2D texture0;',
        'uniform sampler2DArray texture_arr;',
        'in GS_OUT { vec2 vUvs; float sprite_idx; } fs_in;'
    ]

    texture0: _gl.uniform[_gl.sampler2D] = field(default=_gl.uniform.default(), init=False)
    texture_arr: _gl.uniform[_gl.sampler2DArray] = field(default=_gl.uniform.default(), init=False)

    fs_in: _gl.in_t[GS_OUT] = field(default=_gl.in_t.default(), init=False)

    shaders: EntryFragFnGP = field(default_factory=tuple)

    def _assemble(self, config: CompilerConfig.Config = CompilerConfig.default()) -> str:
        if not self._assemble_cache:
            self._assemble_cache = self._preamble(config) + self._header(config)
            self._assemble_cache += compile_mixed_shaders(self.shaders, lambda: self, config)
        return self._assemble_cache


@dataclass
class GS_OUT_V:
    vLumaUvs: _gl.vec2
    vChromaUvs: _gl.vec2


@dataclass
class VideoFragShader(BasicUniform, ShaderModule):

    __kind__: tp.ClassVar[ShaderKind] = 'FragShader'

    __header__: tp.ClassVar[list[str]] = [
        'uniform float time;',
        'uniform float global_time;',
        'uniform float local_duration;',
        'uniform float fps;',
        'uniform vec2 resolution;',
        'uniform sampler2D textureY;',
        'uniform sampler2D textureCb;',
        'uniform sampler2D textureCr;',
        'in GS_OUT {',
        '    vec2 vLumaUvs;',
        '    vec2 vChromaUvs;',
        '} fs_in;'
    ]

    textureY: _gl.uniform[_gl.sampler2D] = field(default=_gl.uniform.default(), init=False)
    textureCb: _gl.uniform[_gl.sampler2D] = field(default=_gl.uniform.default(), init=False)
    textureCr: _gl.uniform[_gl.sampler2D] = field(default=_gl.uniform.default(), init=False)

    fs_in: _gl.in_t[GS_OUT_V] = field(default=_gl.in_t.default(), init=False)

    shaders: EntryFragFnGP = field(default_factory=tuple)

    def _assemble(self, config: CompilerConfig.Config = CompilerConfig.default()) -> str:
        if not self._assemble_cache:
            self._assemble_cache = self._preamble(config) + self._header(config)
            self._assemble_cache += compile_mixed_shaders(self.shaders, lambda: self, config)
        return self._assemble_cache


@dataclass
class VS_OUT:
    vUvs: _gl.vec2
    sprite_idx: float


@dataclass
class PolygonShader(BasicUniform, ShaderModule):

    __kind__: tp.ClassVar[ShaderKind] = 'PolygonShader'

    __header__: tp.ClassVar[list[str]] = [
        'uniform float time;',
        'uniform float global_time;',
        'uniform float local_duration;',
        'uniform float fps;',
        'uniform vec2 resolution;',
        'out VS_OUT {',
        '    vec2 vUvs;',
        '    float sprite_idx;',
        '} vs_out;'
    ]

    vs_out: _gl.out_t[VS_OUT] = field(default=_gl.out_t.default(), init=False)

    shaders: EntryPolyFnGP = field(default_factory=tuple)

    def _assemble(self, config: CompilerConfig.Config = CompilerConfig.default()) -> str:
        if not self._assemble_cache:
            self._assemble_cache = self._preamble(config) + self._header(config)
            self._assemble_cache += compile_mixed_shaders(self.shaders, lambda: self, config)
        return self._assemble_cache


@dataclass
class VideoPolygonShader(BasicUniform, ShaderModule):

    __kind__: tp.ClassVar[ShaderKind] = 'PolygonShader'

    __header__: tp.ClassVar[list[str]] = [
        'uniform float time;',
        'uniform float global_time;',
        'uniform float local_duration;',
        'uniform float fps;',
        'uniform vec2 resolution;',
    ]

    shaders: EntryPolyFnGP = field(default_factory=tuple)

    def _assemble(self, config: CompilerConfig.Config = CompilerConfig.default()) -> str:
        if not self._assemble_cache:
            self._assemble_cache = self._preamble(config) + self._header(config)
            self._assemble_cache += compile_mixed_shaders(self.shaders, lambda: self, config)
        return self._assemble_cache
