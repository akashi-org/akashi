from __future__ import annotations

from .items import CompilerContext, CompileError, GLSLFunc

import typing as tp
import ast
import base64

MANGLE_HEADER = '__pysl_outer_'


def eval_node(node: ast.expr, ctx: CompilerContext) -> str:

    eval_body = ast.unparse(node)
    eval_res = eval(eval_body, ctx.global_symbol, ctx.eval_local_symbol)
    if isinstance(eval_res, bool):
        return str(eval_res).lower()
    return str(eval_res)


def eval_expr_str(expr_str: str, ctx: CompilerContext) -> str:
    eval_res = eval(expr_str, ctx.global_symbol, ctx.eval_local_symbol)
    if isinstance(eval_res, bool):
        return str(eval_res).lower()
    return str(eval_res)


def eval_glsl_fns(glsl_fns: list[GLSLFunc]) -> list[str]:
    res: list[str] = []
    for glsl_fn in glsl_fns:
        r: str = glsl_fn.src
        for k, v in zip(glsl_fn.outer_expr_keys, glsl_fn.outer_expr_values):
            r = r.replace(k, v)
        res.append(r)

    return res


def eval_entry_glsl_fns(glsl_fns: list[GLSLFunc]) -> list[str]:
    res: list[str] = []
    for glsl_fn in glsl_fns:
        if len(glsl_fn.func_decl) > 0 and not glsl_fn.is_entry:
            res.append(glsl_fn.func_decl)

    for glsl_fn in glsl_fns:
        r: str = glsl_fn.src
        for k, v in zip(glsl_fn.outer_expr_keys, glsl_fn.outer_expr_values):
            r = r.replace(k, v)
        res.append(r)

    return res


def mangle_outer_expr(eval_body: str) -> str:
    b64str = base64.b64encode(eval_body.encode('utf-8'))
    return MANGLE_HEADER + b64str.decode('utf-8')


def demangle_outer_expr(mangled: str) -> str:
    if not mangled.startswith(MANGLE_HEADER):
        raise CompileError('Invalid argument provided')

    orig_str = base64.b64decode(mangled.split(MANGLE_HEADER)[1])
    return orig_str.decode('utf-8')
