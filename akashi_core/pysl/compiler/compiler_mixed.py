# pyright: reportPrivateUsage=false
from __future__ import annotations
from dataclasses import dataclass
import typing as tp

from .items import CompilerConfig, CompileError, CompilerContext, _TGLSL
from .utils import can_import3
from .compiler_named import compile_named_shader, to_shader_kind, compile_named_entry_shader_partial, mangled_func_name
from .compiler_inline import compile_inline_shader_partial

import inspect
import re
import ast
import sys

if tp.TYPE_CHECKING:
    from akashi_core.pysl.shader import ShaderModule, ShaderKind, TEntryFn, TNarrowEntryFnOpaque


def entry_point(kind: 'ShaderKind', func_body: str, self_postfix: str = '', next_postfix: str = '') -> str:
    # [TODO] merge defs with the one in compile_inline.py

    if kind == 'FragShader':
        chain_str = '' if len(next_postfix) == 0 else f'frag_main{next_postfix}(color);'
        return f'void frag_main{self_postfix}(inout vec4 color)' + '{' + func_body + chain_str + '}'
    elif kind == 'PolygonShader':
        chain_str = '' if len(next_postfix) == 0 else f'poly_main{next_postfix}(pos);'
        return f'void poly_main{self_postfix}(inout vec3 pos)' + '{' + func_body + chain_str + '}'
    else:
        raise NotImplementedError()


def compile_mixed_shaders(
        fns: tuple['TEntryFn' | 'TNarrowEntryFnOpaque', ...],
        sh_mod_fn: tp.Callable[[], 'ShaderModule'],
        config: CompilerConfig.Config = CompilerConfig.default()) -> _TGLSL:

    kind = sh_mod_fn().__kind__

    if len(fns) == 0:
        return entry_point(kind, '')

    stmts = []
    imported_named_shader_fns_dict = {}

    ctx = CompilerContext(config)

    for idx, fn in enumerate(tp.cast(tuple['TEntryFn', ...], fns)):

        stmt = ''

        ctx.global_symbol = {}
        ctx.cls_symbol = {}
        ctx.eval_local_symbol = {}
        ctx.symbol = {}
        ctx.lambda_args = {}
        ctx.imported_func_symbol = {}

        if fn.__name__ == '<lambda>':
            stmt, imported_named_shader_fns = compile_inline_shader_partial(
                fn, sh_mod_fn, ctx)
        else:
            stmt, imported_named_shader_fns = compile_named_entry_shader_partial(
                tp.cast('TNarrowEntryFnOpaque', fn), ctx)

        for imp_fn in imported_named_shader_fns:
            imported_named_shader_fns_dict[mangled_func_name(ctx, imp_fn)] = imp_fn

        self_postfix = f'_{idx}' if idx > 0 else ''
        next_postfix = f'_{idx + 1}' if idx != len(fns) - 1 else ''

        stmts.insert(0, entry_point(kind, stmt, self_postfix, next_postfix))

    imported_strs = []

    for imp_fn in imported_named_shader_fns_dict.values():
        imp_shader_kind = to_shader_kind(tp.cast(tuple, inspect.getfullargspec(imp_fn).defaults)[0])
        if not can_import3(kind, imp_shader_kind):
            raise CompileError(f'Forbidden import {imp_shader_kind} from {kind}')
        imported_strs.append(compile_named_shader(imp_fn, ctx.config))

    return "".join(imported_strs) + ''.join(stmts)