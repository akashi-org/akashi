# pyright: reportPrivateUsage=false
from __future__ import annotations

from .items import CompilerConfig, CompileError, CompilerContext, _TGLSL, GLSLFunc, CompileCache
from .utils import (
    _get_source,
    get_function_def,
    can_import,
    has_params_qualifier,
    entry_point,
    unwrap_shader_func,
    mangled_func_name,
    get_shader_kind_from_buffer,
    get_hash
)
from .converter import type_converter, body_converter
from .ast import compile_expr, compile_stmt, from_arguments, is_named_func
from .symbol import collect_global_symbols, collect_all_local_symbols
from .evaluator import demangle_outer_expr, eval_expr_str

from akashi_core.pysl import _gl

import typing as tp
import inspect
import ast
import sys
from dataclasses import asdict

if tp.TYPE_CHECKING:
    from akashi_core.pysl.shader import ShaderKind, _TEntryFnOpaque


def _to_shader_kind(kind_str: str) -> 'ShaderKind':

    match kind_str:
        case 'any':
            return 'AnyShader'
        case 'frag':
            return 'FragShader'
        case 'poly':
            return 'PolygonShader'
        case _:
            raise CompileError(f'Invalid shader kind {kind_str} found')


def _collect_argument_symbols(ctx: CompilerContext, deco_fn: tp.Callable):

    anno_items = list(deco_fn.__annotations__.items())
    if len(anno_items) == 0:
        return

    # [TODO] impl type checks later
    # if isinstance(anno_items[0][1], BaseBuffer):
    if (buffer_name := anno_items[0][0]) and buffer_name == 'buffer':
        buffer_type = anno_items[0][1]
        ctx.buffers.append((buffer_name, buffer_type))


def _collect_entry_argument_symbols(ctx: CompilerContext, deco_fn: tp.Callable, kind: 'ShaderKind'):

    anno_items = list(deco_fn.__annotations__.items())
    if len(anno_items) < 2:
        return

    var_name = anno_items[1][0]
    # [TODO] maybe we should use a common name instead of `lambda_args`
    if kind == 'FragShader' and var_name != 'color':
        ctx.lambda_args[var_name] = 'color'
    elif kind == 'PolygonShader' and var_name != 'pos':
        ctx.lambda_args[var_name] = 'pos'


def _compile_func_all(node: ast.FunctionDef, ctx: CompilerContext, func_name: str) -> tuple[_TGLSL, _TGLSL]:

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


def _compile_func_body(node: ast.FunctionDef, ctx: CompilerContext) -> _TGLSL:

    ctx.top_indent = node.col_offset
    if not node.returns:
        raise CompileError('Return type annotation not found')

    body_strs = []
    for stmt in node.body:
        out = compile_stmt(stmt, ctx)
        body_strs.append(out.content)

    return "".join(body_strs)


_ImportedShaderFns = list[tp.Callable]
_DepsShaders = list[GLSLFunc]


def _get_imported_mangled_func_name(fn: tp.Callable, config: CompilerConfig.Config) -> str:

    deco_fn = unwrap_shader_func(fn)
    if not deco_fn:
        raise CompileError('Named shader function must be decorated properly')
    return mangled_func_name(config, deco_fn, False)


def _get_imported_mangled_func_names(fns: _ImportedShaderFns, config: CompilerConfig.Config) -> list[str]:

    return [_get_imported_mangled_func_name(fn, config) for fn in fns]


def _compile_shader_partial(
        fn: tp.Callable,
        config: CompilerConfig.Config,
        is_entry: bool,
        cache: CompileCache | None = None) -> tuple[GLSLFunc, _ImportedShaderFns, _DepsShaders]:

    ctx = CompilerContext(config)

    deco_fn = unwrap_shader_func(fn)
    if not deco_fn:
        raise CompileError('Named shader function must be decorated properly')

    main_func_name = mangled_func_name(ctx.config, deco_fn, False)
    main_has_cache = False
    main_cached_glsl_fn = None
    root = None
    py_src = None

    if cache and main_func_name in cache.fn_map:
        if not cache.fn_dirty_map[main_func_name]:
            main_has_cache = True
            main_cached_glsl_fn = cache.fn_map[main_func_name]
        else:
            py_src, root = _get_source(deco_fn, config)
            cur_hash = get_hash(py_src)
            if cur_hash == cache.fn_map[main_func_name].orig_src_hash:
                main_has_cache = True
                main_cached_glsl_fn = cache.fn_map[main_func_name]
                cache.fn_dirty_map[main_func_name] = False

        # Skip compiling when all srcs including the main and the deps have no outer exprs
        if main_has_cache and main_cached_glsl_fn and len(main_cached_glsl_fn.outer_expr_keys) == 0:
            res_glsl: list[GLSLFunc] = []
            for imp_fname in main_cached_glsl_fn.imported_mangled_func_names:
                if imp_fname not in cache.fn_map:
                    break
                if cache.fn_dirty_map[imp_fname]:
                    break
                if len(cache.fn_map[imp_fname].outer_expr_keys) > 0:
                    break
                res_glsl.append(cache.fn_map[imp_fname])
            else:
                return (main_cached_glsl_fn, [], res_glsl)

    shader_kind = _to_shader_kind(tp.cast(tuple, inspect.getfullargspec(fn).defaults)[0])
    if not shader_kind:
        raise CompileError('Named shader function must be decorated with its shader kind')
    collect_global_symbols(ctx, deco_fn)
    _collect_argument_symbols(ctx, deco_fn)
    if is_entry:
        _collect_entry_argument_symbols(ctx, deco_fn, shader_kind)
    imported_named_shader_fns = collect_all_local_symbols(ctx, deco_fn, cache)

    main_glsl_fn: GLSLFunc
    if main_has_cache and main_cached_glsl_fn:
        new_outer_expr_values = [eval_expr_str(demangle_outer_expr(k), ctx)
                                 for k in main_cached_glsl_fn.outer_expr_keys]
        main_glsl_fn = GLSLFunc(**asdict(main_cached_glsl_fn) | {'outer_expr_values': new_outer_expr_values})
    else:
        if not py_src:
            py_src, root = _get_source(deco_fn, ctx.config, cache)

        if not root:
            root = ast.parse(py_src)

        func_def = get_function_def(root)
        if not is_named_func(func_def, ctx.global_symbol):
            raise CompileError('Named shader function must be decorated properly')

        main_func_def = ''
        main_func_decl = ''
        if is_entry:
            main_func_def = _compile_func_body(func_def, ctx)
        else:
            main_func_def, main_func_decl = _compile_func_all(func_def, ctx, main_func_name)

        outer_expr_keys, outer_expr_values = (
            zip(*ctx.outer_expr_map.items()) if len(ctx.outer_expr_map) > 0 else ((), ())
        )

        main_glsl_fn = GLSLFunc(
            src=main_func_def,
            orig_src=py_src,
            orig_src_hash=get_hash(py_src),
            mangled_func_name=main_func_name,
            func_decl=main_func_decl,
            is_entry=is_entry,
            shader_kind=shader_kind,
            outer_expr_keys=outer_expr_keys,
            outer_expr_values=outer_expr_values,
            imported_mangled_func_names=tuple(_get_imported_mangled_func_names(imported_named_shader_fns, ctx.config))
        )

        if cache:
            # cache update
            cache.fn_map[main_func_name] = main_glsl_fn
            cache.fn_dirty_map[main_func_name] = False

    return (main_glsl_fn, imported_named_shader_fns, [])


def compile_lib_shader(
        fn: tp.Callable,
        config: CompilerConfig.Config,
        cache: CompileCache | None = None) -> list[GLSLFunc]:

    main_glsl, imported_named_shader_fns, deps_glsls = _compile_shader_partial(fn, config, False, cache)
    if len(imported_named_shader_fns) == 0:
        return deps_glsls + [main_glsl]
    else:
        glsl_fns: list[GLSLFunc] = []
        for imp_fn in imported_named_shader_fns:
            imp_shader_kind = _to_shader_kind(tp.cast(tuple, inspect.getfullargspec(imp_fn).defaults)[0])
            if not can_import(main_glsl.shader_kind, imp_shader_kind):
                raise CompileError(f'Forbidden import {imp_shader_kind} from {main_glsl.shader_kind}')
            glsl_fns += compile_lib_shader(imp_fn, config, cache)

        return glsl_fns + [main_glsl]


def compile_entry_shaders(
        fns: tuple['_TEntryFnOpaque', ...],
        buffer_type: tp.Type[_gl._buffer_type],
        config: CompilerConfig.Config,
        cache: CompileCache | None = None) -> list[GLSLFunc]:

    assert len(fns) > 0

    kind = get_shader_kind_from_buffer(buffer_type)

    entry_glsl_fns: list[GLSLFunc] = []
    imported_named_shader_fns_dict = {}
    imported_glsl_fns: list[GLSLFunc] = []

    ctx = CompilerContext(config)

    for idx, fn in enumerate(fns):

        entry_glsl_fn = None

        ctx.clear_symbols()

        entry_glsl_fn, imported_named_shader_fns, deps_glsls = _compile_shader_partial(
            tp.cast(tp.Callable, fn), config, True, cache
        )
        if len(imported_named_shader_fns) == 0:
            imported_glsl_fns += deps_glsls
        else:
            for imp_fn in imported_named_shader_fns:
                imported_named_shader_fns_dict[mangled_func_name(ctx.config, imp_fn)] = imp_fn

        self_postfix = f'_{idx}' if idx > 0 else ''
        next_postfix = f'_{idx + 1}' if idx != len(fns) - 1 else ''

        entry_glsl_fn = GLSLFunc(**asdict(entry_glsl_fn) |
                                 {'src': entry_point(kind, entry_glsl_fn.src, self_postfix, next_postfix)})
        entry_glsl_fns.insert(0, entry_glsl_fn)

    for imp_fn in imported_named_shader_fns_dict.values():
        imp_fn_name = _get_imported_mangled_func_name(imp_fn, config)
        for imp_glsl in imported_glsl_fns:
            if imp_fn_name == imp_glsl.mangled_func_name:
                continue
        imp_shader_kind = _to_shader_kind(tp.cast(tuple, inspect.getfullargspec(imp_fn).defaults)[0])
        if not can_import(kind, imp_shader_kind):
            raise CompileError(f'Forbidden import {imp_shader_kind} from {kind}')
        imported_glsl_fns += compile_lib_shader(imp_fn, ctx.config, cache)

    res_glsl_fns: list[GLSLFunc] = []

    out_func_names = []
    imported_decls = []
    imported_defs = []
    for glsl_fn in imported_glsl_fns:
        if glsl_fn.mangled_func_name not in out_func_names:
            res_glsl_fns.append(glsl_fn)
            out_func_names.append(glsl_fn.mangled_func_name)

    return res_glsl_fns + entry_glsl_fns
