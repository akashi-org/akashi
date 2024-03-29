# pyright: reportPrivateUsage=false
from __future__ import annotations

from .items import CompilerContext, CompileCache
from .utils import (
    unwrap_shader_func,
    mangled_func_name,
    get_function_def,
    get_ast,
    resolve_module,
    get_shader_kind_from_buffer
)
from .ast import is_named_func
from akashi_core.pysl import _gl

from types import ModuleType
import typing as tp
import inspect
import sys
import ast

if tp.TYPE_CHECKING:
    from akashi_core.pysl.shader import ShaderCompiler


def collect_global_symbols(ctx: CompilerContext, fn: tp.Callable):

    ctx.global_symbol = vars(sys.modules[fn.__module__])


# Expects collect_global_symbols() to be executed before
def collect_all_local_symbols(
        ctx: CompilerContext,
        deco_fn: tp.Callable,
        cache: CompileCache | None = None) -> list[tp.Callable]:

    imported_named_shader_fn = []

    # [TODO] Perhaps we can optimize this func with the shader cache?

    for local_varname in deco_fn.__code__.co_names:
        if local_varname in ctx.global_symbol:
            value = ctx.global_symbol[local_varname]
            if callable(value) and (_fn := unwrap_shader_func(value)):
                if is_named_func(get_function_def(get_ast(_fn, ctx.config, cache)), ctx.global_symbol):
                    # [TODO] really we need to insert a wrapped func?
                    imported_named_shader_fn.append(value)
                    ctx.imported_func_symbol[local_varname] = mangled_func_name(ctx.config, _fn)
            elif isinstance(value, ModuleType):
                for attr_name in resolve_module(local_varname, deco_fn, ctx.config, cache):
                    wrap_fn = getattr(value, attr_name)
                    if inspect.isfunction(wrap_fn) and (inner_fn := unwrap_shader_func(wrap_fn)):
                        if is_named_func(get_function_def(get_ast(inner_fn, ctx.config, cache)), ctx.global_symbol):
                            imported_named_shader_fn.append(wrap_fn)
                            ctx.imported_func_symbol[attr_name] = mangled_func_name(ctx.config, inner_fn)

    # variables which are referenced by deco_fn's closure
    for idx, freevar in enumerate(deco_fn.__code__.co_freevars):

        if not(hasattr(deco_fn, '__closure__')) or not(deco_fn.__closure__):
            continue

        value = deco_fn.__closure__[idx].cell_contents

        if callable(value) and (_fn := unwrap_shader_func(value)):
            if is_named_func(get_function_def(get_ast(_fn, ctx.config, cache)), ctx.global_symbol):
                imported_named_shader_fn.append(value)
                ctx.imported_func_symbol[freevar] = mangled_func_name(ctx.config, _fn)
                continue

        ctx.eval_local_symbol[freevar] = value

    return imported_named_shader_fn


# Expects collect_global_symbols() to be executed before
def collect_eval_local_symbols(
        ctx: CompilerContext,
        deco_fn: tp.Callable,
        cache: CompileCache | None = None):

    # variables which are referenced by deco_fn's closure
    for idx, freevar in enumerate(deco_fn.__code__.co_freevars):

        if not(hasattr(deco_fn, '__closure__')) or not(deco_fn.__closure__):
            continue

        value = deco_fn.__closure__[idx].cell_contents

        if callable(value) and (_fn := unwrap_shader_func(value)):
            if is_named_func(get_function_def(get_ast(_fn, ctx.config, cache)), ctx.global_symbol):
                continue

        ctx.eval_local_symbol[freevar] = value
