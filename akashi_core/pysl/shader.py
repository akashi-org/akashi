# pyright: reportPrivateUsage=false
from __future__ import annotations
from .compiler.items import CompilerConfig, CompileCache
from .compiler.compiler import compile_entry_shaders
from .compiler.evaluator import eval_entry_glsl_fns
from . import _gl

import typing as tp
from dataclasses import dataclass, field

ShaderKind = tp.Literal['AnyShader', 'FragShader', 'PolygonShader', 'GeomShader']


_T_co = tp.TypeVar('_T_co', covariant=True)


class _TEntryFnOpaque(tp.Generic[_T_co]):
    ...


_NamedEntryFragFn = tp.Callable[[_gl._TFragBuffer, _gl.frag_color], None]
_NamedEntryPolyFn = tp.Callable[[_gl._TPolyBuffer, _gl.poly_pos], None]

_COMPILE_CONFIG = CompilerConfig.default()
_COMPILE_CACHE = CompileCache(config=_COMPILE_CONFIG)


def _invalidate_compile_cache():
    global _COMPILE_CACHE
    _COMPILE_CACHE = CompileCache(config=_COMPILE_CONFIG)


@dataclass
class ShaderCompiler:
    __glsl_version__: tp.ClassVar[str] = '#version 420 core\n'

    shaders: tuple
    buffer_type: tp.Type[_gl._buffer_type]
    header: list[str] = field(default_factory=list)
    preamble: tuple[str, ...] = field(default_factory=tuple)

    def _header(self, config: CompilerConfig.Config) -> list[str]:
        if not config['pretty_compile']:
            return self.header
        else:
            return [s + '\n' for s in self.header]

    def _preamble(self, config: CompilerConfig.Config) -> list[str]:
        if len(self.preamble) == 0:
            return [self.__glsl_version__]
        else:
            if not config['pretty_compile']:
                return [self.__glsl_version__] + [p + '\n' for p in self.preamble]
            else:
                return [self.__glsl_version__] + [p + '\n' for p in self.preamble] + ['\n']

    def _assemble(self) -> str:
        config = _COMPILE_CONFIG
        artifacts = self._preamble(config) + self._header(config)
        artifacts += eval_entry_glsl_fns(compile_entry_shaders(self.shaders,
                                                               self.buffer_type, config, _COMPILE_CACHE))
        return ''.join(artifacts)


_frag_shader_header: list[str] = [
    'uniform float time;',
    'uniform float global_time;',
    'uniform float local_duration;',
    'uniform float fps;',
    'uniform vec2 resolution;',
    'uniform vec2 mesh_size;',
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
    'uniform vec2 mesh_size;',
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
    'uniform vec2 mesh_size;',
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
    'uniform vec2 mesh_size;',
    'out VS_OUT {',
    '    vec2 vLumaUvs;',
    '    vec2 vChromaUvs;',
    '} vs_out;'
]
