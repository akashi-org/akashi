from __future__ import annotations
import typing as tp
from dataclasses import dataclass, field

if tp.TYPE_CHECKING:
    from akashi_core.pysl.shader import ShaderCompiler, ShaderKind


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
    top_indent: int = field(default=0, init=False)

    global_symbol: dict = field(default_factory=dict)
    eval_local_symbol: dict = field(default_factory=dict)
    lambda_args: dict = field(default_factory=dict)
    imported_func_symbol: dict = field(default_factory=dict)

    buffers: list = field(default_factory=list[tuple])

    def clear_symbols(self):
        self.global_symbol = {}
        self.eval_local_symbol = {}
        self.lambda_args = {}
        self.imported_func_symbol = {}

        self.buffers = []
