# pyright: reportPrivateUsage=false
from __future__ import annotations
from .compiler.items import CompilerConfig
from .compiler.compiler import compile_shaders
from . import _gl

import typing as tp
from dataclasses import dataclass, field

ShaderKind = tp.Literal['AnyShader', 'FragShader', 'PolygonShader', 'GeomShader']

LEntryFragFn = tp.Callable[[tp.Type[_gl._expr], _gl._TFragBuffer, _gl.frag_color], _gl._expr]
LEntryPolyFn = tp.Callable[[tp.Type[_gl._expr], _gl._TPolyBuffer, _gl.poly_pos], _gl._expr]


_T_co = tp.TypeVar('_T_co', covariant=True)


class _TEntryFnOpaque(tp.Generic[_T_co]):
    ...


_NamedEntryFragFn = tp.Callable[[_gl._TFragBuffer, _gl.frag_color], None]
_NamedEntryPolyFn = tp.Callable[[_gl._TPolyBuffer, _gl.poly_pos], None]


@dataclass
class ShaderCompiler:
    __glsl_version__: tp.ClassVar[str] = '#version 420 core\n'
    _assemble_cache: tp.Optional[str] = field(default=None, init=False)

    shaders: tuple
    buffer_type: tp.Type[_gl._buffer_type]
    header: list[str] = field(default_factory=list)
    preamble: tuple[str, ...] = field(default_factory=tuple)

    def _header(self, config: CompilerConfig.Config) -> str:
        if not config['pretty_compile']:
            return ''.join(self.header)
        else:
            return '\n'.join(self.header) + '\n'

    def _preamble(self, config: CompilerConfig.Config) -> str:
        if len(self.preamble) == 0:
            return self.__glsl_version__
        else:
            if not config['pretty_compile']:
                return self.__glsl_version__ + '\n'.join(self.preamble) + '\n'
            else:
                return self.__glsl_version__ + '\n'.join(self.preamble) + '\n' + '\n'

    def _invalidate_cache(self):
        self._assemble_cache = None

    def _assemble(self, config: CompilerConfig.Config = CompilerConfig.default()) -> str:
        if not self._assemble_cache:
            self._assemble_cache = self._preamble(config) + self._header(config)
            self._assemble_cache += compile_shaders(self.shaders, self.buffer_type, config)
        return self._assemble_cache


_frag_shader_header: list[str] = [
    'uniform float time;',
    'uniform float global_time;',
    'uniform float local_duration;',
    'uniform float fps;',
    'uniform vec2 resolution;',
    'uniform sampler2D texture0;',
    'uniform sampler2DArray texture_arr;',
    'in GS_OUT { vec2 vUvs; float sprite_idx; } fs_in;'
]


_video_frag_shader_header: list[str] = [
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


_poly_shader_header: list[str] = [
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

_video_poly_shader_header: list[str] = [
    'uniform float time;',
    'uniform float global_time;',
    'uniform float local_duration;',
    'uniform float fps;',
    'uniform vec2 resolution;',
    'out VS_OUT {',
    '    vec2 vLumaUvs;',
    '    vec2 vChromaUvs;',
    '} vs_out;'
]
