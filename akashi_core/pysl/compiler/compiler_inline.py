# pyright: reportPrivateUsage=false
from __future__ import annotations
from dataclasses import dataclass
import typing as tp

from akashi_core.pysl import _gl
from .items import CompilerConfig, CompileError, CompilerContext, _TGLSL
from .ast import compile_stmt, compile_expr, compile_shader_staticmethod
from .utils import can_import2
from .symbol import instance_symbol_analysis

import inspect
import re
import ast
import sys

if tp.TYPE_CHECKING:
    from akashi_core.pysl.shader import ShaderKind, EntryFragFn, EntryPolyFn

# [XXX] circular import?
from akashi_core.pysl.shader import FragShader, PolygonShader


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
    # [TODO] maybe we need to exclude comments or something like that
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

    # [TODO] we should resolve the alias here

    # [TODO] any better ways?
    head = raw_expr_src.split('(')[0]
    if head.endswith('expr'):
        return parse_expr_level1(raw_expr_src, ctx)
    elif head.endswith('assign'):
        return parse_assign_level1(raw_expr_src, ctx)
    elif head.endswith('let'):
        raise NotImplementedError()
    else:
        raise CompileError('parse_expr() failed')


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


def parse_assign_level1(assign_src: str, ctx: CompilerContext) -> str:
    '''
        from: gl.assign(AAABBBB).eq(PPP)
        to: AAABBBB, '=', PPP

        from: gl.assign(AAABBBB).op('+=',PPP)
        to: AAABBBB, '+=', PPP
    '''

    root = ast.parse(assign_src.strip())

    local_ctx = {'content': ''}

    class Visitor(ast.NodeVisitor):

        def visit_Call(self, node: ast.Call):
            lhs = compile_expr(tp.cast(ast.Call, tp.cast(ast.Attribute, node.func).value).args[0], ctx).content
            op = str(tp.cast(ast.Attribute, node.func).attr)

            if op == 'eq':
                rhs = compile_expr(node.args[0], ctx).content
                local_ctx['content'] = f'{lhs} = {rhs}'
            elif op == 'op':
                op_str = compile_expr(node.args[0], ctx).content
                rhs = compile_expr(node.args[1], ctx).content
                local_ctx['content'] = f'{lhs} {op_str} {rhs}'

    Visitor().visit(root)

    return local_ctx['content'] + ';'


def collect_symbols(ctx: CompilerContext, fn: tp.Union['EntryFragFn', 'EntryPolyFn'], kind: 'ShaderKind'):

    ctx.global_symbol = vars(sys.modules[fn.__module__])

    for idx, freevar in enumerate(fn.__code__.co_freevars):
        ctx.local_symbol[freevar] = fn.__closure__[idx].cell_contents

    buf_arg, var_arg = inspect.getfullargspec(fn).args
    ctx.lambda_args[buf_arg] = ''
    if kind == 'FragShader':
        ctx.lambda_args[var_arg] = '_fragColor'
        instance_symbol_analysis(FragShader(), ctx)
    elif kind == 'PolygonShader':
        ctx.lambda_args[var_arg] = 'pos'
        instance_symbol_analysis(PolygonShader(), ctx)
    else:
        raise NotImplementedError()


def compile_inline_shader(
        fn: tp.Union['EntryFragFn', 'EntryPolyFn'],
        kind: 'ShaderKind',
        config: CompilerConfig.Config = CompilerConfig.default()) -> _TGLSL:

    res = []

    src = get_inline_source(fn)
    exprs = split_exprs(src)

    ctx = CompilerContext(config)
    collect_symbols(ctx, fn, kind)

    for expr in exprs:
        res.append(parse_expr(expr, ctx))

    imported_strs = []
    for imp in ctx.imported_current.values():
        if not can_import2(kind, imp[0]):
            raise CompileError(f'Forbidden import {imp[0].__kind__} from {kind}')

        imported_strs.append(compile_shader_staticmethod(imp[0], imp[1], True, ctx.config))

    imported = "".join(imported_strs)

    return imported + entry_point(kind, ''.join(res))
