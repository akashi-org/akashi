from __future__ import annotations
from .compiler import compile_shader_module, CompilerConfig
from . import _gl

from abc import abstractmethod, ABCMeta
import typing as tp
from dataclasses import dataclass, field


@dataclass
class ShaderModule(metaclass=ABCMeta):
    __glsl_version__: tp.ClassVar[str] = '#version 420 core\n'
    __header__: tp.ClassVar[list[str]] = []
    __preamble__: tp.ClassVar[list[str]] = []

    _assemble_cache: tp.Optional[str] = field(default=None, init=False)

    time: _gl.uniform[float] = field(default=_gl.uniform.default(), init=False)
    global_time: _gl.uniform[float] = field(default=_gl.uniform.default(), init=False)
    local_duration: _gl.uniform[float] = field(default=_gl.uniform.default(), init=False)
    fps: _gl.uniform[float] = field(default=_gl.uniform.default(), init=False)
    resolution: _gl.uniform[_gl.vec2] = field(default=_gl.uniform.default(), init=False)

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
class FragShader(ShaderModule):

    __header__: tp.ClassVar[list[str]] = [
        'uniform float time;',
        'uniform float global_time;',
        'uniform float local_duration;',
        'uniform float fps;',
        'uniform vec2 resolution;',
        'uniform sampler2D texture0;',
        'in GS_OUT { vec2 vUvs; } fs_in;'
    ]

    texture0: _gl.uniform[_gl.sampler2D] = field(default=_gl.uniform.default(), init=False)

    @_gl.method
    @abstractmethod
    def frag_main(self, color: _gl.inout_p[_gl.vec4]) -> None: ...

    def _assemble(self, config: CompilerConfig.Config = CompilerConfig.default()) -> str:
        if not self._assemble_cache:
            self._assemble_cache = self._preamble(config) + self._header(config) + compile_shader_module(self, config)
        return self._assemble_cache


@dataclass
class VideoFragShader(ShaderModule):

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

    # textureY: _gl.uniform[_gl.sampler2D] = _gl.uniform.default()
    # textureCb: _gl.uniform[_gl.sampler2D] = _gl.uniform.default()
    # textureCr: _gl.uniform[_gl.sampler2D] = _gl.uniform.default()

    @_gl.method
    @abstractmethod
    def frag_main(self, color: _gl.inout_p[_gl.vec4]) -> None: ...

    def _assemble(self, config: CompilerConfig.Config = CompilerConfig.default()) -> str:
        if not self._assemble_cache:
            self._assemble_cache = self._preamble(config) + self._header(config) + compile_shader_module(self, config)
        return self._assemble_cache


@dataclass
class PolygonShader(ShaderModule):

    __header__: tp.ClassVar[list[str]] = [
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

    __header__: tp.ClassVar[list[str]] = [
        'uniform float time;',
        'uniform float global_time;',
        'uniform float local_duration;',
        'uniform float fps;',
        'uniform vec2 resolution;',
        'uniform sampler2D texture0;',
        'in VS_OUT { vec2 vUvs; } gs_in[];',
        'out GS_OUT { vec2 vUvs; } gs_out;',
    ]

    texture0: _gl.uniform[_gl.sampler2D] = field(default=_gl.uniform.default(), init=False)

    @_gl.method
    @abstractmethod
    def main(self) -> None: ...

    def _assemble(self, config: CompilerConfig.Config = CompilerConfig.default()) -> str:
        if not self._assemble_cache:
            self._assemble_cache = self._preamble(config) + self._header(config) + compile_shader_module(self, config)
        return self._assemble_cache


@dataclass
class VideoGeomShader(ShaderModule):

    __header__: tp.ClassVar[list[str]] = [
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

    # textureY: _gl.uniform[_gl.sampler2D] = _gl.uniform.default()
    # textureCb: _gl.uniform[_gl.sampler2D] = _gl.uniform.default()
    # textureCr: _gl.uniform[_gl.sampler2D] = _gl.uniform.default()

    @_gl.method
    @abstractmethod
    def main(self) -> None: ...

    def _assemble(self, config: CompilerConfig.Config = CompilerConfig.default()) -> str:
        if not self._assemble_cache:
            self._assemble_cache = self._preamble(config) + self._header(config) + compile_shader_module(self, config)
        return self._assemble_cache
