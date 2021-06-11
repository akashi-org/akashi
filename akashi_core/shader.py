from __future__ import annotations
from typing import Literal, Generic, TypeVar, cast
from dataclasses import dataclass, field


_TShader = TypeVar('_TShader')


@dataclass(frozen=True)
class AnyShader:
    src: str


@dataclass(frozen=True)
class _Shader:
    entry: str
    includes: list[AnyShader] = field(default_factory=list)
    src: str = ''


@dataclass(frozen=True)
class FragShader(_Shader):

    type: Literal['frag'] = field(default='frag', init=False)

    def __rshift__(self, other: 'FragShader') -> 'ShaderModule[FragShader]':
        return ShaderModule(self.type, (self, other))

    def _assemble(self, cur_entry: str = 'frag_main', next_entry: str = '') -> str:
        meta_header = '#version 420'
        chain_header = '' if len(next_entry) < 1 else f'void {next_entry}(inout vec4);\n'
        chain_call = '' if len(next_entry) < 1 else f'{next_entry}(_fragColor);\n'
        comp_entry = f'{chain_header} void {cur_entry}(inout vec4 _fragColor){{ {self.entry}; {chain_call} }}'
        return "\n".join(
            [meta_header] + [i.src for i in self.includes] + [self.src] + [comp_entry]
        )


@dataclass(frozen=True)
class GeomShader(_Shader):

    type: Literal['geom'] = field(default='geom', init=False)

    def _assemble(self) -> str:
        meta_header = '#version 420'
        comp_entry = f'void main(){{ {self.entry}; }}'
        return "\n".join(
            [meta_header] + [i.src for i in self.includes] + [self.src] + [comp_entry]
        )


@dataclass(frozen=True)
class ShaderModule(Generic[_TShader]):
    type: Literal['frag']
    shaders: tuple[_TShader, ...] = field(default_factory=tuple)

    def __rshift__(self, other: _TShader) -> 'ShaderModule[_TShader]':
        return ShaderModule(self.type, (*self.shaders, other))

    def _assemble(self) -> list[str]:
        # Normally this always returns false, but for just in case.
        if self.type != 'frag':
            raise NotImplementedError()

        _shaders = cast(tuple['FragShader', ...], self.shaders)
        comp_srcs: list[str] = []
        for idx, shader in enumerate(_shaders):
            cur_entry = 'frag_main' if idx == 0 else f'frag_main{idx}'
            next_entry = '' if idx == len(_shaders) - 1 else f'frag_main{idx+1}'
            comp_srcs.append(shader._assemble(cur_entry, next_entry))
        return comp_srcs
