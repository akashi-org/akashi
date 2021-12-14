# pyright: reportPrivateUsage=false
from __future__ import annotations

from .items import CompilerConfig, CompileError, CompilerContext, _TGLSL
from .utils import get_source, get_function_def, get_class_def
from .transformer import type_transformer
from .ast import compile_expr, from_FunctionDef, from_ClassDef
from .symbol import global_symbol_analysis, instance_symbol_analysis

import typing as tp
import inspect
import ast
import sys

if tp.TYPE_CHECKING:
    from akashi_core.pysl.shader import ShaderModule


def __unwrap_shader_func(fn: tp.Callable) -> tp.Optional[ast.FunctionDef]:

    if not(hasattr(fn, '__closure__') and fn.__closure__ and len(fn.__closure__) > 0):
        return None

    deco_fn = fn.__closure__[0].cell_contents

    py_src = get_source(deco_fn)
    root = ast.parse(py_src)
    func_def = get_function_def(root)

    ctx = CompilerContext(CompilerConfig.default())

    if len(func_def.decorator_list) == 0:
        return None

    for deco in func_def.decorator_list:
        deco_node = compile_expr(deco, ctx)
        deco_tpname = type_transformer(deco_node.content, ctx, deco_node.node)
        if deco_tpname in ['method', 'func', 'test_func']:
            return func_def

    return None


def compile_shader_func(
        fn: tp.Callable,
        config: CompilerConfig.Config = CompilerConfig.default()) -> _TGLSL:

    ctx = CompilerContext(config)

    func_def = __unwrap_shader_func(fn)
    if not func_def:
        raise CompileError('Shader function/method must be decorated properly')

    out = from_FunctionDef(func_def, ctx)
    return out.content


def compile_shader_module(
        sh_mod: 'ShaderModule',
        config: CompilerConfig.Config = CompilerConfig.default()) -> _TGLSL:
    klass = sh_mod.__class__

    py_src = get_source(klass)
    root = ast.parse(py_src)
    class_def = get_class_def(root)

    ctx = CompilerContext(config)

    ctx.shmod_name = klass.__name__
    ctx.shmod_inst = sh_mod
    ctx.shmod_klass = klass
    global_symbol_analysis(klass, ctx)
    instance_symbol_analysis(sh_mod, ctx)

    out = from_ClassDef(class_def, ctx)

    return out.content
