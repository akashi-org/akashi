# pyright: reportPrivateUsage=false
from __future__ import annotations

from .items import CompilerConfig, CompileError, CompilerContext, _TGLSL, GLSLFunc
from .utils import (
    get_source,
    get_function_def,
    can_import,
    has_params_qualifier,
    entry_point,
    unwrap_shader_func,
    mangled_func_name,
    get_shader_kind_from_buffer,
    resolve_module
)
from .converter import type_converter, body_converter
from .ast import compile_expr, compile_stmt, from_arguments, is_named_func
from .symbol import collect_global_symbols, collect_local_symbols

from akashi_core.pysl import _gl

import typing as tp
import inspect
import ast
import sys

if tp.TYPE_CHECKING:
    from akashi_core.pysl.shader import ShaderKind, _TEntryFnOpaque


def to_shader_kind(kind_str: str) -> 'ShaderKind':

    match kind_str:
        case 'any':
            return 'AnyShader'
        case 'frag':
            return 'FragShader'
        case 'poly':
            return 'PolygonShader'
        case _:
            raise CompileError(f'Invalid shader kind {kind_str} found')


def collect_argument_symbols(ctx: CompilerContext, deco_fn: tp.Callable):

    anno_items = list(deco_fn.__annotations__.items())
    if len(anno_items) == 0:
        return

    # [TODO] impl type checks later
    # if isinstance(anno_items[0][1], BaseBuffer):
    if (buffer_name := anno_items[0][0]) and buffer_name == 'buffer':
        buffer_type = anno_items[0][1]
        ctx.buffers.append((buffer_name, buffer_type))


def collect_entry_argument_symbols(ctx: CompilerContext, deco_fn: tp.Callable, kind: 'ShaderKind'):

    anno_items = list(deco_fn.__annotations__.items())
    if len(anno_items) < 2:
        return

    var_name = anno_items[1][0]
    # [TODO] maybe we should use a common name instead of `lambda_args`
    if kind == 'FragShader' and var_name != 'color':
        ctx.lambda_args[var_name] = 'color'
    elif kind == 'PolygonShader' and var_name != 'pos':
        ctx.lambda_args[var_name] = 'pos'


def compile_func_all(node: ast.FunctionDef, ctx: CompilerContext, func_name: str) -> tuple[_TGLSL, _TGLSL]:

    ctx.top_indent = node.col_offset

    if not node.returns:
        raise CompileError('Return type annotation not found')

    returns = compile_expr(node.returns, ctx)
    returns_str = type_converter(returns.content, ctx, returns.node)

    if has_params_qualifier(returns_str):
        raise CompileError('Return type must not have its parameter qualifier')

    args = from_arguments(node.args, ctx, 0)
    args_temp = []
    for idx, arg in enumerate(args):
        arg_tpname = type_converter(arg.content, ctx, arg.node)
        args_temp.append(f'{arg_tpname} {arg.node.arg}')

    args_str = ', '.join(args_temp)

    body_str = body_converter(node.body, ctx, brace_on_ellipsis=False)

    return (
        f'{returns_str} {func_name}({args_str}){body_str}',
        f'{returns_str} {func_name}({args_str});',
    )


def compile_func_body(node: ast.FunctionDef, ctx: CompilerContext) -> _TGLSL:

    ctx.top_indent = node.col_offset
    if not node.returns:
        raise CompileError('Return type annotation not found')

    body_strs = []
    for stmt in node.body:
        out = compile_stmt(stmt, ctx)
        body_strs.append(out.content)

    return "".join(body_strs)


def compile_named_shader(
        fn: tp.Callable,
        config: CompilerConfig.Config = CompilerConfig.default()) -> list[GLSLFunc]:

    glsl_fns: list[GLSLFunc] = []

    ctx = CompilerContext(config)

    deco_fn = unwrap_shader_func(fn)
    if not deco_fn:
        raise CompileError('Named shader function must be decorated properly')

    py_src = get_source(deco_fn)
    root = ast.parse(py_src)

    shader_kind = to_shader_kind(tp.cast(tuple, inspect.getfullargspec(fn).defaults)[0])
    if not shader_kind:
        raise CompileError('Named shader function must be decorated with its shader kind')
    collect_global_symbols(ctx, deco_fn)
    collect_argument_symbols(ctx, deco_fn)
    imported_named_shader_fns = collect_local_symbols(ctx, deco_fn)

    func_def = get_function_def(root)
    if not is_named_func(func_def, ctx.global_symbol):
        raise CompileError('Named shader function must be decorated properly')

    main_func_name = mangled_func_name(ctx, deco_fn, False)
    main_func_def, main_func_decl = compile_func_all(func_def, ctx, main_func_name)

    for imp_fn in imported_named_shader_fns:
        imp_shader_kind = to_shader_kind(tp.cast(tuple, inspect.getfullargspec(imp_fn).defaults)[0])
        if not can_import(shader_kind, imp_shader_kind):
            raise CompileError(f'Forbidden import {imp_shader_kind} from {shader_kind}')
        glsl_fns += compile_named_shader(imp_fn, ctx.config)

    glsl_fns.append(GLSLFunc(
        src=main_func_def,
        mangled_func_name=main_func_name,
        func_decl=main_func_decl,
        outer_expr_map=ctx.outer_expr_map
    ))

    return glsl_fns


def compile_named_entry_shader_partial(
        fn: '_TEntryFnOpaque', ctx: CompilerContext) -> tuple[GLSLFunc, list[tp.Callable]]:

    deco_fn = unwrap_shader_func(tp.cast(tp.Callable, fn))
    if not deco_fn:
        raise CompileError('Named shader function must be decorated properly')

    py_src = get_source(deco_fn)
    root = ast.parse(py_src)

    shader_kind = to_shader_kind(tp.cast(tuple, inspect.getfullargspec(fn).defaults)[0])
    if not shader_kind:
        raise CompileError('Named shader function must be decorated with its shader kind')
    collect_global_symbols(ctx, deco_fn)
    collect_argument_symbols(ctx, deco_fn)
    collect_entry_argument_symbols(ctx, deco_fn, shader_kind)
    imported_named_shader_fns = collect_local_symbols(ctx, deco_fn)

    func_def = get_function_def(root)
    if not is_named_func(func_def, ctx.global_symbol):
        raise CompileError('Named shader function must be decorated properly')

    func_body = compile_func_body(func_def, ctx)

    entry_glsl_fn = GLSLFunc(
        src=func_body,
        mangled_func_name='',
        func_decl='',
        is_entry=True,
        outer_expr_map=ctx.outer_expr_map
    )

    return (entry_glsl_fn, imported_named_shader_fns)


def compile_shaders(
        fns: tuple,
        buffer_type: tp.Type[_gl._buffer_type],
        config: CompilerConfig.Config = CompilerConfig.default()) -> list[GLSLFunc]:

    kind = get_shader_kind_from_buffer(buffer_type)

    if len(fns) == 0:
        return [GLSLFunc(src=entry_point(kind, ''), mangled_func_name='', func_decl='', is_entry=True)]

    entry_glsl_fns: list[GLSLFunc] = []
    imported_named_shader_fns_dict = {}

    ctx = CompilerContext(config)

    for idx, fn in enumerate(fns):

        entry_glsl_fn = None

        ctx.clear_symbols()

        entry_glsl_fn, imported_named_shader_fns = compile_named_entry_shader_partial(
            tp.cast('_TEntryFnOpaque', fn), ctx)

        for imp_fn in imported_named_shader_fns:
            imported_named_shader_fns_dict[mangled_func_name(ctx, imp_fn)] = imp_fn

        self_postfix = f'_{idx}' if idx > 0 else ''
        next_postfix = f'_{idx + 1}' if idx != len(fns) - 1 else ''

        entry_glsl_fn.src = entry_point(kind, entry_glsl_fn.src, self_postfix, next_postfix)
        entry_glsl_fns.insert(0, entry_glsl_fn)

    imported_glsl_fns: list[GLSLFunc] = []

    for imp_fn in imported_named_shader_fns_dict.values():
        imp_shader_kind = to_shader_kind(tp.cast(tuple, inspect.getfullargspec(imp_fn).defaults)[0])
        if not can_import(kind, imp_shader_kind):
            raise CompileError(f'Forbidden import {imp_shader_kind} from {kind}')
        imported_glsl_fns += compile_named_shader(imp_fn, ctx.config)

    res_glsl_fns: list[GLSLFunc] = []

    out_func_names = []
    imported_decls = []
    imported_defs = []
    for glsl_fn in imported_glsl_fns:
        if glsl_fn.mangled_func_name not in out_func_names:
            res_glsl_fns.append(glsl_fn)
            out_func_names.append(glsl_fn.mangled_func_name)

    return res_glsl_fns + entry_glsl_fns
