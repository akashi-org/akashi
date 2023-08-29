# pyright: reportPrivateUsage=false
from __future__ import annotations
from .compiler.items import CompilerConfig, CompileCache
from .compiler.compiler import compile_entry_shaders, _get_imported_mangled_func_names
from .compiler.evaluator import eval_entry_glsl_fns
from . import _gl

import typing as tp
from dataclasses import dataclass, field

ShaderKind = tp.Literal['AnyShader', 'FragShader', 'PolygonShader', 'GeomShader']

ShaderMemoType = list[tp.Hashable] | None


_T_co = tp.TypeVar('_T_co', covariant=True)


class _TEntryFnOpaque(tp.Generic[_T_co]):
    ...


_NamedEntryFragFn = tp.Callable[[_gl._frag, _gl.frag_color], None]
_NamedEntryPolyFn = tp.Callable[[_gl._poly, _gl.poly_pos], None]

_COMPILE_CONFIG = CompilerConfig.default()
_COMPILE_CACHE = CompileCache(config=_COMPILE_CONFIG)

_ARTIFACT_CACHE = {}


def _invalidate_compile_cache():
    global _COMPILE_CACHE
    _COMPILE_CACHE = CompileCache(config=_COMPILE_CONFIG)


def _invalidate_artifact_cache():
    global _ARTIFACT_CACHE
    _ARTIFACT_CACHE = {}


@ dataclass
class ShaderCompiler:
    __glsl_version__: tp.ClassVar[str] = '#version 420 core\n'

    shaders: tuple
    buffer_type: tp.Type[_gl._buffer_type]
    header: list[str] = field(default_factory=list)
    preamble: tuple[str, ...] = field(default_factory=tuple)
    memo: ShaderMemoType = None

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

    def _compile(self, config: CompilerConfig.Config) -> str:
        artifacts = self._preamble(config) + self._header(config)
        artifacts += eval_entry_glsl_fns(compile_entry_shaders(self.shaders,
                                                               self.buffer_type, config, _COMPILE_CACHE))
        return ''.join(artifacts)

    def _assemble(self) -> str:
        if self.memo is None:
            return self._compile(_COMPILE_CONFIG)

        fn_names = _get_imported_mangled_func_names(list(self.shaders), _COMPILE_CONFIG)
        a_key = (tuple(fn_names), tuple(self.memo))
        if a_key in _ARTIFACT_CACHE:
            return _ARTIFACT_CACHE[a_key]
        else:
            _ARTIFACT_CACHE[a_key] = self._compile(_COMPILE_CONFIG)
            return _ARTIFACT_CACHE[a_key]


_uniform_shader_header: list[str] = [
    'uniform float time;',
    'uniform float global_time;',
    'uniform float local_duration;',
    'uniform float fps;',
    'uniform vec2 resolution;',
    'uniform vec2 mesh_size;',
    'uniform sampler2D unit_texture0;',
    'uniform sampler2D text_texture0;',
    'uniform sampler2D shape_texture0;',
    'uniform sampler2DArray image_textures;',
    'uniform sampler2D textureY;',
    'uniform sampler2D textureCb;',
    'uniform sampler2D textureCr;',
]

_frag_shader_header: list[str] = [
    *_uniform_shader_header,
    'in GS_OUT {',
    '    vec2 video_luma_uv;',
    '    vec2 video_chroma_uv;',
    '    vec2 uv;',
    '    float sprite_idx;',
    '} fs_in;'
]


_poly_shader_header: list[str] = [
    *_uniform_shader_header,
    'out VS_OUT {',
    '    vec2 video_luma_uv;',
    '    vec2 video_chroma_uv;',
    '    vec2 uv;',
    '    float sprite_idx;',
    '} vs_out;'
]
