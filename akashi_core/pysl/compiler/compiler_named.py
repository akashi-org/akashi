# pyright: reportPrivateUsage=false
from __future__ import annotations

from .items import CompilerConfig, CompileError, CompilerContext, _TGLSL
from .utils import get_source, get_function_def, can_import3, has_params_qualifier
from .transformer import type_transformer, body_transformer
from .ast import compile_expr, compile_stmt, from_arguments
from .symbol import global_symbol_analysis, instance_symbol_analysis

from types import ModuleType

import typing as tp
import inspect
import ast
import sys

if tp.TYPE_CHECKING:
    from akashi_core.pysl.shader import ShaderKind, ShaderModule, TEntryFn, TNarrowEntryFnOpaque


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
                ctx.cls_symbol[attr_name] = (basic_tpname, attr_type)
            else:
                ctx.cls_symbol[attr_name] = (str(type(attr_type).__name__), attr_type)


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


# Expects collect_global_symbols() to be executed before
def collect_local_symbols(ctx: CompilerContext, deco_fn: tp.Callable) -> list[tp.Callable]:

    imported_named_shader_fn = []

    for local_varname in deco_fn.__code__.co_names:
        if local_varname in ctx.global_symbol:
            value = ctx.global_symbol[local_varname]
            if callable(value) and (_fn := unwrap_shader_func(value)):
                if is_named_func(get_function_def(ast.parse(get_source(_fn))), ctx.global_symbol):
                    # [TODO] really we need to insert a wrapped func?
                    imported_named_shader_fn.append(value)
                    ctx.imported_func_symbol[local_varname] = mangled_func_name(ctx, _fn)
            elif isinstance(value, ModuleType):
                for attr_name in resolve_module(local_varname, deco_fn):
                    wrap_fn = getattr(value, attr_name)
                    if inspect.isfunction(wrap_fn) and (inner_fn := unwrap_shader_func(wrap_fn)):
                        if is_named_func(get_function_def(ast.parse(get_source(inner_fn))), ctx.global_symbol):
                            imported_named_shader_fn.append(wrap_fn)
                            ctx.imported_func_symbol[attr_name] = mangled_func_name(ctx, inner_fn)

    # variables which are referenced by deco_fn's closure
    for idx, freevar in enumerate(deco_fn.__code__.co_freevars):
        value = deco_fn.__closure__[idx].cell_contents

        if callable(value) and (_fn := unwrap_shader_func(value)):
            if is_named_func(get_function_def(ast.parse(get_source(_fn))), ctx.global_symbol):
                imported_named_shader_fn.append(value)
                # [TODO] really we need this?
                ctx.imported_func_symbol[freevar] = mangled_func_name(ctx, _fn)
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
        if deco_tpname in ['fn', 'entry_frag', 'entry_poly']:
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


def mangled_func_name(ctx: CompilerContext, deco_fn: tp.Callable, unwrap: bool = True):

    if unwrap and (_fn := unwrap_shader_func(deco_fn)):
        deco_fn = _fn

    prefix = get_mangle_prefix(ctx, deco_fn)
    if len(prefix) == 0:
        return deco_fn.__name__
    else:
        return prefix + '_' + deco_fn.__name__


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
    collect_argument_symbols(ctx, deco_fn)
    imported_named_shader_fns = collect_local_symbols(ctx, deco_fn)

    func_def = get_function_def(root)
    if not is_named_func(func_def, ctx.global_symbol):
        raise CompileError('Named shader function must be decorated properly')

    out = parse_func_all(func_def, ctx, mangled_func_name(ctx, deco_fn, False))

    imported_strs = []
    for imp_fn in imported_named_shader_fns:
        imp_shader_kind = to_shader_kind(tp.cast(tuple, inspect.getfullargspec(imp_fn).defaults)[0])
        if not can_import3(shader_kind, imp_shader_kind):
            raise CompileError(f'Forbidden import {imp_shader_kind} from {shader_kind}')
        imported_strs.append(compile_named_shader(imp_fn, ctx.config))

    return "".join(imported_strs) + out


def parse_func_all(node: ast.FunctionDef, ctx: CompilerContext, func_name: str) -> _TGLSL:

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
        ctx.symbol[arg.node.arg] = arg_tpname
        args_temp.append(f'{arg_tpname} {arg.node.arg}')

    args_str = ', '.join(args_temp)

    body_str = body_transformer(node.body, ctx, brace_on_ellipsis=False)

    return f'{returns_str} {func_name}({args_str}){body_str}'


def parse_func_body(node: ast.FunctionDef, ctx: CompilerContext) -> _TGLSL:

    is_method = len(ctx.buffers) > 0

    ctx.top_indent = node.col_offset
    if not node.returns:
        raise CompileError('Return type annotation not found')

    args = from_arguments(node.args, ctx, 1 if is_method else 0)
    for idx, arg in enumerate(args):
        arg_tpname = type_transformer(arg.content, ctx, arg.node)
        ctx.symbol[arg.node.arg] = arg_tpname

    body_strs = []
    for stmt in node.body:
        out = compile_stmt(stmt, ctx)
        body_strs.append(out.content)

    return "".join(body_strs)


def compile_named_entry_shaders(
        fns: tuple['TNarrowEntryFnOpaque', ...],
        config: CompilerConfig.Config = CompilerConfig.default()) -> _TGLSL:

    if len(fns) == 0:
        return ''

    shader_kind = to_shader_kind(tp.cast(tuple, inspect.getfullargspec(fns[0]).defaults)[0])
    if not shader_kind:
        raise CompileError('Named shader function must be decorated with its shader kind')

    ctx = CompilerContext(config)
    ctx.shader_kind = shader_kind

    stmts = []
    imported_named_shader_fns_dict = {}

    for idx, fn in enumerate(fns):

        deco_fn = unwrap_shader_func(tp.cast(tp.Callable, fn))
        if not deco_fn:
            raise CompileError('Named shader function must be decorated properly')

        py_src = get_source(deco_fn)
        root = ast.parse(py_src)

        ctx.global_symbol = {}
        ctx.cls_symbol = {}
        ctx.eval_local_symbol = {}
        ctx.symbol = {}
        ctx.lambda_args = {}
        ctx.imported_func_symbol = {}

        collect_global_symbols(ctx, deco_fn)
        collect_argument_symbols(ctx, deco_fn)
        collect_entry_argument_symbols(ctx, deco_fn, shader_kind)
        imported_named_shader_fns = collect_local_symbols(ctx, deco_fn)
        for imp_fn in imported_named_shader_fns:
            imported_named_shader_fns_dict[mangled_func_name(ctx, imp_fn)] = imp_fn

        func_def = get_function_def(root)
        if not is_named_func(func_def, ctx.global_symbol):
            raise CompileError('Named shader function must be decorated properly')

        self_postfix = f'_{idx}' if idx > 0 else ''
        next_postfix = f'_{idx + 1}' if idx != len(fns) - 1 else ''

        func_body = parse_func_body(func_def, ctx)
        stmts.insert(0, entry_point(shader_kind, ''.join(func_body), self_postfix, next_postfix))

    imported_strs = []
    for imp_fn in imported_named_shader_fns_dict.values():
        imp_shader_kind = to_shader_kind(tp.cast(tuple, inspect.getfullargspec(imp_fn).defaults)[0])
        if not can_import3(shader_kind, imp_shader_kind):
            raise CompileError(f'Forbidden import {imp_shader_kind} from {shader_kind}')
        imported_strs.append(compile_named_shader(imp_fn, ctx.config))

    return "".join(imported_strs) + ''.join(stmts)


def compile_named_entry_shader_partial(fn: 'TNarrowEntryFnOpaque', ctx: CompilerContext) -> tuple[_TGLSL, list[tp.Callable]]:

    deco_fn = unwrap_shader_func(tp.cast(tp.Callable, fn))
    if not deco_fn:
        raise CompileError('Named shader function must be decorated properly')

    py_src = get_source(deco_fn)
    root = ast.parse(py_src)

    shader_kind = to_shader_kind(tp.cast(tuple, inspect.getfullargspec(fn).defaults)[0])
    if not shader_kind:
        raise CompileError('Named shader function must be decorated with its shader kind')
    ctx.shader_kind = shader_kind
    collect_global_symbols(ctx, deco_fn)
    collect_argument_symbols(ctx, deco_fn)
    collect_entry_argument_symbols(ctx, deco_fn, shader_kind)
    imported_named_shader_fns = collect_local_symbols(ctx, deco_fn)

    func_def = get_function_def(root)
    if not is_named_func(func_def, ctx.global_symbol):
        raise CompileError('Named shader function must be decorated properly')

    func_body = parse_func_body(func_def, ctx)

    return (func_body, imported_named_shader_fns)
