# pyright: reportPrivateUsage=false
from __future__ import annotations

from .items import CompilerConfig, CompileError, CompilerContext, _TGLSL
from .utils import (
    get_source,
    get_function_def,
    can_import,
    has_params_qualifier,
    entry_point,
    unwrap_shader_func,
    mangled_func_name,
    resolve_module
)
from .transformer import type_transformer, body_transformer
from .ast import compile_expr, compile_stmt, from_arguments, is_named_func
from .symbol import collect_global_symbols, collect_local_symbols


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

        for attr_name in resolve_module(buffer_name, deco_fn):
            attr_type = getattr(buffer_type, attr_name)
            if isinstance(attr_type, tp._GenericAlias):  # type: ignore
                basic_tpname: str = str(attr_type.__origin__.__name__)  # type:ignore
                ctx.local_symbol[attr_name] = basic_tpname
            else:
                ctx.local_symbol[attr_name] = str(type(attr_type).__name__)


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


def compile_func_all(node: ast.FunctionDef, ctx: CompilerContext, func_name: str) -> _TGLSL:

    ctx.top_indent = node.col_offset

    if not node.returns:
        raise CompileError('Return type annotation not found')

    returns = compile_expr(node.returns, ctx)
    returns_str = type_transformer(returns.content, ctx, returns.node)

    if has_params_qualifier(returns_str):
        raise CompileError('Return type must not have its parameter qualifier')

    args = from_arguments(node.args, ctx, 0)
    args_temp = []
    for idx, arg in enumerate(args):
        arg_tpname = type_transformer(arg.content, ctx, arg.node)
        ctx.local_symbol[arg.node.arg] = arg_tpname
        args_temp.append(f'{arg_tpname} {arg.node.arg}')

    args_str = ', '.join(args_temp)

    body_str = body_transformer(node.body, ctx, brace_on_ellipsis=False)

    return f'{returns_str} {func_name}({args_str}){body_str}'


def compile_func_body(node: ast.FunctionDef, ctx: CompilerContext) -> _TGLSL:

    is_method = len(ctx.buffers) > 0

    ctx.top_indent = node.col_offset
    if not node.returns:
        raise CompileError('Return type annotation not found')

    args = from_arguments(node.args, ctx, 1 if is_method else 0)
    for idx, arg in enumerate(args):
        arg_tpname = type_transformer(arg.content, ctx, arg.node)
        ctx.local_symbol[arg.node.arg] = arg_tpname

    body_strs = []
    for stmt in node.body:
        out = compile_stmt(stmt, ctx)
        body_strs.append(out.content)

    return "".join(body_strs)


def compile_named_shader(
        fn: tp.Callable,
        config: CompilerConfig.Config = CompilerConfig.default()) -> _TGLSL:

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

    out = compile_func_all(func_def, ctx, mangled_func_name(ctx, deco_fn, False))

    imported_strs = []
    for imp_fn in imported_named_shader_fns:
        imp_shader_kind = to_shader_kind(tp.cast(tuple, inspect.getfullargspec(imp_fn).defaults)[0])
        if not can_import(shader_kind, imp_shader_kind):
            raise CompileError(f'Forbidden import {imp_shader_kind} from {shader_kind}')
        imported_strs.append(compile_named_shader(imp_fn, ctx.config))

    return "".join(imported_strs) + out


def compile_named_entry_shader_partial(
        fn: '_TEntryFnOpaque', ctx: CompilerContext) -> tuple[_TGLSL, list[tp.Callable]]:

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

    return (func_body, imported_named_shader_fns)
