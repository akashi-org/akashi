from __future__ import annotations
from typing import Literal, Generic, TypeVar, Union
from dataclasses import dataclass, field


_TShaderType = TypeVar('_TShaderType', Literal['frag'], Literal['geom'], Literal['any'])
_TShaderLayerType = TypeVar('_TShaderLayerType', Literal['video'], Literal['novideo'])


@dataclass(frozen=True)
class LibShader(Generic[_TShaderType]):
    type: _TShaderType
    includes: tuple[Union[LibShader[_TShaderType], LibShader[Literal['any']]], ...] = field(default_factory=tuple)
    src: str = ''

    def _assemble(self) -> str:
        return "\n".join(
            [inc._assemble() for inc in self.includes] + [self.src]
        )


@dataclass(frozen=True)
class EntryShader(Generic[_TShaderType, _TShaderLayerType]):
    type: _TShaderType
    layer_type: _TShaderLayerType
    includes: tuple[Union[LibShader[_TShaderType], LibShader[Literal['any']]], ...] = field(default_factory=tuple)
    src: str = ''

    def _assemble(self) -> str:
        meta_header = '#version 420 core'

        uniform_header = '''
            uniform float time;
            uniform float global_time;
            uniform float local_duration;
            uniform float fps;
            uniform vec2 resolution;
        '''

        if self.layer_type == 'video':
            uniform_header += '''
                uniform sampler2D textureY;
                uniform sampler2D textureCb;
                uniform sampler2D textureCr;
            '''
        else:
            uniform_header += '''
                uniform sampler2D texture0;
            '''

        inout_header = ''
        if self.type == 'frag':
            if self.layer_type == 'video':
                inout_header += '''
                    in GS_OUT {
                        vec2 vLumaUvs;
                        vec2 vChromaUvs;
                    } fs_in;
                '''
            else:
                inout_header += '''
                    in GS_OUT {
                        vec2 vUvs;
                    } fs_in;
                '''

        return "\n".join(
            [meta_header, uniform_header, inout_header] + [inc._assemble() for inc in self.includes] + [self.src]
        )
