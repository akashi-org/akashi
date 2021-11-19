from __future__ import annotations
from .compiler import compile_shader_module, CompilerConfig
from . import _gl

from typing import Final, Optional
from abc import abstractmethod, ABCMeta


class ShaderModule(metaclass=ABCMeta):
    __glsl_version__: Final[str] = '#version 420 core\n'
    __preamble__: list[str] = []
    __header__: list[str] = []

    def __init__(self):
        self._assemble_cache: Optional[str] = None

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


class FragShader(ShaderModule):

    __header__ = [
        'uniform float time;',
        'uniform float global_time;',
        'uniform float local_duration;',
        'uniform float fps;',
        'uniform vec2 resolution;',
        'uniform sampler2D texture0;',
        'in GS_OUT { vec2 vUvs; } fs_in;'
    ]

    @_gl.method
    @abstractmethod
    def frag_main(self, color: _gl.inout_p[_gl.vec4]) -> None: ...

    def _assemble(self, config: CompilerConfig.Config = CompilerConfig.default()) -> str:
        if not self._assemble_cache:
            self._assemble_cache = self._preamble(config) + self._header(config) + compile_shader_module(self, config)
        return self._assemble_cache


class VideoFragShader(ShaderModule):

    __header__ = [
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

    @_gl.method
    @abstractmethod
    def frag_main(self, color: _gl.inout_p[_gl.vec4]) -> None: ...

    def _assemble(self, config: CompilerConfig.Config = CompilerConfig.default()) -> str:
        if not self._assemble_cache:
            self._assemble_cache = self._preamble(config) + self._header(config) + compile_shader_module(self, config)
        return self._assemble_cache


class PolygonShader(ShaderModule):

    __header__ = [
        'uniform float time;',
        'uniform float global_time;',
        'uniform float local_duration;',
        'uniform float fps;',
        'uniform vec2 resolution;'
    ]

    @_gl.method
    @abstractmethod
    def poly_main(self, position: _gl.inout_p[_gl.vec4]) -> None: ...

    def _assemble(self, config: CompilerConfig.Config = CompilerConfig.default()) -> str:
        if not self._assemble_cache:
            self._assemble_cache = self._preamble(config) + self._header(config) + compile_shader_module(self, config)
        return self._assemble_cache


class GeomShader(ShaderModule):

    __header__ = [
        'uniform float time;',
        'uniform float global_time;',
        'uniform float local_duration;',
        'uniform float fps;',
        'uniform vec2 resolution;',
        'uniform sampler2D texture0;',
        'in VS_OUT { vec2 vUvs; } gs_in[];',
        'out GS_OUT { vec2 vUvs; } gs_out;',
    ]

    @_gl.method
    @abstractmethod
    def main(self) -> None: ...

    def _assemble(self, config: CompilerConfig.Config = CompilerConfig.default()) -> str:
        if not self._assemble_cache:
            self._assemble_cache = self._preamble(config) + self._header(config) + compile_shader_module(self, config)
        return self._assemble_cache


class VideoGeomShader(ShaderModule):

    __header__ = [
        'uniform float time;',
        'uniform float global_time;',
        'uniform float local_duration;',
        'uniform float fps;',
        'uniform vec2 resolution;',
        'uniform sampler2D textureY;',
        'uniform sampler2D textureCb;',
        'uniform sampler2D textureCr;',
        'in VS_OUT {',
        '    vec2 vLumaUvs;',
        '    vec2 vChromaUvs;',
        '}',
        'gs_in[];',
        'out GS_OUT {',
        '    vec2 vLumaUvs;',
        '    vec2 vChromaUvs;',
        '}',
        'gs_out;',
    ]

    @_gl.method
    @abstractmethod
    def main(self) -> None: ...

    def _assemble(self, config: CompilerConfig.Config = CompilerConfig.default()) -> str:
        if not self._assemble_cache:
            self._assemble_cache = self._preamble(config) + self._header(config) + compile_shader_module(self, config)
        return self._assemble_cache
