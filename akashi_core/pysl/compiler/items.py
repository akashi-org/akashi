from __future__ import annotations
import typing as tp
from dataclasses import dataclass, field

if tp.TYPE_CHECKING:
    from akashi_core.pysl.shader import ShaderModule, ShaderKind


_TGLSL = str


class CompileError(Exception):
    pass


class CompilerConfig:

    class Config(tp.TypedDict):
        ''' '''

        ''' If True, compiler tries to output more human-readable shader code'''
        pretty_compile: bool

        mangle_mode: tp.Literal['hard', 'soft', 'none']

    @staticmethod
    def default() -> Config:

        return {
            'pretty_compile': False,
            'mangle_mode': 'hard'
        }


@dataclass
class CompilerContext:
    config: CompilerConfig.Config
    symbol: dict = field(default_factory=dict)
    cls_symbol: dict = field(default_factory=dict)
    global_symbol: dict = field(default_factory=dict)
    eval_local_symbol: dict = field(default_factory=dict)
    lambda_args: dict = field(default_factory=dict)
    imported: set = field(default_factory=set)
    imported_current: dict = field(default_factory=dict)
    on_import_resolution: bool = False
    top_indent: int = field(default=0, init=False)
    shmod_name: str = field(default='', init=False)
    shmod_inst: tp.Optional['ShaderModule'] = field(default=None, init=False)
    shmod_klass: tp.Optional[tp.Type['ShaderModule']] = field(default=None, init=False)

    # named shader
    shader_kind: 'ShaderKind' = 'AnyShader'
