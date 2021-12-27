# pyright: reportPrivateUsage=false
from __future__ import annotations

from .items import CompileError, CompilerContext
from akashi_core.pysl import _gl

import typing as tp
import inspect
import ast
import re

if tp.TYPE_CHECKING:
    from akashi_core.pysl.shader import ShaderCompiler, ShaderKind


def get_source(obj) -> str:
    raw_src = inspect.getsource(obj)

    # raw_src can be indented already, and that will cause IndentationError in ast.parse().
    # So here we explicitly strip the indents.
    lines = raw_src.split('\n')
    top_indent = m.span()[0] if (m := re.search(r'[^\s]', lines[0])) else 0
    return '\n'.join([ln[top_indent:] for ln in lines])


def get_function_def(root: ast.AST) -> ast.FunctionDef:

    ctx = {'node': None}

    class Visitor(ast.NodeVisitor):

        def __init__(self, _ctx: dict):
            self._ctx = _ctx

        def visit_FunctionDef(self, node: ast.FunctionDef):
            self._ctx['node'] = node

    Visitor(ctx).visit(root)

    if not ctx['node']:
        raise CompileError('ast.FunctionDef not found')
    else:
        return tp.cast(ast.FunctionDef, ctx['node'])


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


def can_import(kind: 'ShaderKind', imp_kind: 'ShaderKind') -> bool:

    if imp_kind == 'AnyShader':
        return True
    elif kind == imp_kind:
        return True
    else:
        return False


def has_params_qualifier(tp_name: str) -> bool:
    return tp_name.startswith('in ') or tp_name.startswith('out ') or tp_name.startswith('inout ')


def is_wrapped_type(tp_name: str) -> bool:
    return (
        tp_name.startswith('in_t') or
        tp_name.startswith('out_t') or
        tp_name.startswith('uniform')
    )


def get_stmt_indent(node: ast.AST, ctx: CompilerContext) -> str:
    return (node.col_offset - ctx.top_indent) * ' ' if ctx.config['pretty_compile'] else ''


def entry_point(kind: 'ShaderKind', func_body: str, self_postfix: str = '', next_postfix: str = '') -> str:

    if kind == 'FragShader':
        chain_str = '' if len(next_postfix) == 0 else f'frag_main{next_postfix}(color);'
        return f'void frag_main{self_postfix}(inout vec4 color)' + '{' + func_body + chain_str + '}'
    elif kind == 'PolygonShader':
        chain_str = '' if len(next_postfix) == 0 else f'poly_main{next_postfix}(pos);'
        return f'void poly_main{self_postfix}(inout vec3 pos)' + '{' + func_body + chain_str + '}'
    else:
        raise NotImplementedError()


def unwrap_shader_func(fn: tp.Callable) -> tp.Callable | None:

    if not(hasattr(fn, '__closure__') and fn.__closure__ and len(fn.__closure__) > 0):
        return None
    return fn.__closure__[0].cell_contents


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


def get_shader_kind_from_buffer(buffer_type: tp.Type[_gl._buffer_type]) -> 'ShaderKind':

    if issubclass(buffer_type, _gl._frag):
        return 'FragShader'
    elif issubclass(buffer_type, _gl._poly):
        return 'PolygonShader'
    else:
        raise NotImplementedError()


def visit_all(node: ast.AST):
    class Visitor(ast.NodeVisitor):
        def visit(self, node):
            print(node)
            super().visit(node)

    Visitor().visit(node)


def dump_ast(node: ast.AST) -> str:
    return ast.dump(node, indent=2)
