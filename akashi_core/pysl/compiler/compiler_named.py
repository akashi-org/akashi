# pyright: reportPrivateUsage=false
from __future__ import annotations

from .items import CompilerConfig, CompileError, CompilerContext, _TGLSL
from .utils import get_source, get_function_def, can_import2, can_import3
from .transformer import type_transformer
from .ast import compile_expr, from_FunctionDef, compile_shader_staticmethod
from .symbol import global_symbol_analysis, instance_symbol_analysis

from types import ModuleType

import typing as tp
import inspect
import ast
import sys

if tp.TYPE_CHECKING:
    from akashi_core.pysl.shader import ShaderKind, ShaderModule


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


def resolve_module(mod_name: str, deco_fn: tp.Callable) -> list[str]:

    attr_names = []

    class Visitor(ast.NodeVisitor):
        def visit_Attribute(self, node: ast.Attribute):
            if isinstance(node.value, ast.Name):
                value_str = str(node.value.id)
                if value_str == mod_name:
                    attr_names.append(str(node.attr))

            super().visit(node.value)

    py_src = get_source(deco_fn)
    root = ast.parse(py_src)

    Visitor().visit(root)

    return attr_names


def collect_global_symbols(ctx: CompilerContext, deco_fn: tp.Callable):

    ctx.global_symbol = vars(sys.modules[deco_fn.__module__])


# Expects collect_global_symbols() to be executed before
def collect_local_symbols(ctx: CompilerContext, deco_fn: tp.Callable) -> list[tp.Callable]:

    imported_named_shader_fn = []

    for local_varname in deco_fn.__code__.co_names:
        if local_varname in ctx.global_symbol:
            value = ctx.global_symbol[local_varname]
            if callable(value) and (_fn := unwrap_shader_func(value)):
                if is_named_func(get_function_def(ast.parse(get_source(_fn))), ctx.global_symbol):
                    imported_named_shader_fn.append(value)
            elif isinstance(value, ModuleType):
                for attr_name in resolve_module(local_varname, deco_fn):
                    wrap_fn = getattr(value, attr_name)
                    inner_fn = unwrap_shader_func(wrap_fn)
                    if is_named_func(get_function_def(ast.parse(get_source(inner_fn))), ctx.global_symbol):
                        imported_named_shader_fn.append(wrap_fn)

    # variables which are referenced by deco_fn's closure
    for idx, freevar in enumerate(deco_fn.__code__.co_freevars):
        value = deco_fn.__closure__[idx].cell_contents

        if callable(value) and (_fn := unwrap_shader_func(value)):
            if is_named_func(get_function_def(ast.parse(get_source(_fn))), ctx.global_symbol):
                imported_named_shader_fn.append(value)
                continue

        ctx.eval_local_symbol[freevar] = value

    return imported_named_shader_fn


def unwrap_shader_func(fn: tp.Callable) -> tp.Callable | None:

    if not(hasattr(fn, '__closure__') and fn.__closure__ and len(fn.__closure__) > 0):
        return None
    return fn.__closure__[0].cell_contents


def is_named_func(func_def: ast.FunctionDef, global_symbol: dict) -> bool:

    ctx = CompilerContext(CompilerConfig.default())
    ctx.global_symbol = global_symbol

    if len(func_def.decorator_list) == 0:
        return False

    for deco in func_def.decorator_list:
        deco_node = compile_expr(deco, ctx)
        deco_tpname = type_transformer(deco_node.content, ctx, deco_node.node)
        if deco_tpname in ['fn']:
            return True

    return False


def get_mangle_prefix(ctx: CompilerContext, deco_fn: tp.Callable):

    full_mod_name = deco_fn.__module__
    simple_mod_name = str(full_mod_name).split('.')[-1]

    local_name = ''
    if deco_fn.__name__ != deco_fn.__qualname__:
        local_name = '_' + deco_fn.__qualname__.replace('.<locals>', '').replace('.', '_')

    match ctx.config['mangle_mode']:
        case 'hard':
            return simple_mod_name + local_name
        case 'soft':
            return simple_mod_name
        case 'none':
            return ''


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
    ctx.shader_kind = shader_kind
    collect_global_symbols(ctx, deco_fn)
    imported_named_shader_fns = collect_local_symbols(ctx, deco_fn)

    func_def = get_function_def(root)
    if not is_named_func(func_def, ctx.global_symbol):
        raise CompileError('Named shader function must be decorated properly')

    out = from_FunctionDef(func_def, ctx, get_mangle_prefix(ctx, deco_fn))

    imported_strs = []
    for imp_fn in imported_named_shader_fns:
        imp_shader_kind = to_shader_kind(tp.cast(tuple, inspect.getfullargspec(imp_fn).defaults)[0])
        if not can_import3(shader_kind, imp_shader_kind):
            raise CompileError(f'Forbidden import {imp_shader_kind} from {shader_kind}')
        imported_strs.append(compile_named_shader(imp_fn, ctx.config))

    return "".join(imported_strs) + out.content
