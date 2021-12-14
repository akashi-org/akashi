# pyright: reportPrivateUsage=false
from __future__ import annotations
from dataclasses import dataclass
import typing as tp

from akashi_core.pysl import _gl
from .items import CompilerConfig, CompileError, CompilerContext, _TGLSL
from .ast import compile_stmt, compile_shader_staticmethod
from .utils import can_import2

import inspect
import re
import ast
import sys

if tp.TYPE_CHECKING:
    from akashi_core.pysl.shader import ShaderKind, EntryFragFn, EntryPolyFn


def entry_point(kind: 'ShaderKind', func_body: str) -> str:

    if kind == 'FragShader':
        return 'void frag_main(inout vec4 _fragColor){' + func_body + '}'
    elif kind == 'PolygonShader':
        return 'void poly_main(inout vec3 pos){' + func_body + '}'
    else:
        raise NotImplementedError()


def get_inline_source(fn: tp.Union['EntryFragFn', 'EntryPolyFn']):

    raw_src = inspect.getsource(fn)
    if fn.__name__ != '<lambda>':
        raise CompileError('As for inline pysl, we currently support lambda function only!')
    else:
        return raw_src.split('lambda')[1:][0].split(':')[1:][0]


def split_exprs(src: str) -> list[str]:
    '''
        from: gl.expr(...) >> gl.let(...) >> ... >> gl.expr(...)
        to:  [gl.expr(...), gl.let(...), ..., gl.expr(...)]
    '''
    return src.split('>>')


def parse_expr(raw_expr_src: str, ctx: CompilerContext) -> str:
    '''
        input: gl.expr(...) or C(...) (where C is an alias for gl.expr)
        pattern: expr

        input: gl.let(...) or C(...) (where C is an alias for gl.let)
        pattern: let

        input: gl.assign(...) or C(...) (where C is an alias for gl.assign)
        pattern: assign
    '''

    # we should resolve the alias here

    return parse_expr_level1(raw_expr_src, ctx)


def parse_expr_level1(expr_src: str, ctx: CompilerContext) -> str:
    '''
        from: gl.expr(AAABBBB)
        to: AAABBBB

        from: C(AAABBBB) (where C is an alias for gl.expr)
        to: AAABBBB
    '''
    t_expr = re.findall(r'(?<=\().*(?=\))', expr_src)[0]
    return parse_expr_level2(t_expr, ctx)


def parse_expr_level2(iexpr_src: str, ctx: CompilerContext) -> str:
    root = ast.parse(iexpr_src)  # expects ast.Module
    return compile_stmt(root.body[0], ctx).content


def collect_symbols(ctx: CompilerContext, fn: tp.Union['EntryFragFn', 'EntryPolyFn']):

    ctx.global_symbol = vars(sys.modules[fn.__module__])

    for idx, freevar in enumerate(fn.__code__.co_freevars):
        ctx.local_symbol[freevar] = fn.__closure__[idx].cell_contents


def compile_inline_shader(
        fn: tp.Union['EntryFragFn', 'EntryPolyFn'],
        kind: 'ShaderKind',
        config: CompilerConfig.Config = CompilerConfig.default()) -> _TGLSL:

    res = []

    src = get_inline_source(fn)
    exprs = split_exprs(src)

    ctx = CompilerContext(config)
    collect_symbols(ctx, fn)

    for expr in exprs:
        res.append(parse_expr(expr, ctx))

    imported_strs = []
    for imp in ctx.imported_current.values():
        if not can_import2(kind, imp[0]):
            raise CompileError(f'Forbidden import {imp[0].__kind__} from {kind}')

        imported_strs.append(compile_shader_staticmethod(imp[0], imp[1], True, ctx.config))

    imported = "".join(imported_strs)

    return imported + entry_point(kind, ';'.join(res))
