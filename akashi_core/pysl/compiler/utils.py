from __future__ import annotations

from .items import CompileError, CompilerContext

import typing as tp
import inspect
import ast
import re

if tp.TYPE_CHECKING:
    from akashi_core.pysl.shader import ShaderModule, ShaderKind


def get_source(obj) -> str:
    raw_src = inspect.getsource(obj)

    # raw_src can be indented already, and that will cause IndentationError in ast.parse().
    # So here we explicitly strip the indents.
    lines = raw_src.split('\n')
    top_indent = m.span()[0] if (m := re.search(r'[^\s]', lines[0])) else 0
    return '\n'.join([ln[top_indent:] for ln in lines])


def unwrap_func(fn: tp.Callable) -> tp.Callable:
    ''' unwrap func decorated with @gl.func, @gl.method '''
    if hasattr(fn, '__closure__') and fn.__closure__ and len(fn.__closure__) > 0:
        return fn.__closure__[0].cell_contents
    else:
        return fn


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


def get_class_def(root: ast.AST) -> ast.ClassDef:

    ctx = {'node': None}

    class Visitor(ast.NodeVisitor):

        def __init__(self, _ctx: dict):
            self._ctx = _ctx

        def visit_ClassDef(self, node: ast.ClassDef):
            self._ctx['node'] = node

    Visitor(ctx).visit(root)

    if not ctx['node']:
        raise CompileError('ast.ClassDef not found')
    else:
        return tp.cast(ast.ClassDef, ctx['node'])


def mangle_shader_func(shmod_name: str, func_name: str) -> str:
    if func_name in ['frag_main', 'main', 'poly_main']:
        return func_name
    else:
        return shmod_name + '_' + func_name


def can_import(from_mod: tp.Type['ShaderModule'], imp_mod: tp.Type['ShaderModule']) -> bool:

    if imp_mod.__kind__ == 'AnyShader':
        return True
    elif from_mod.__kind__ == imp_mod.__kind__:
        return True
    else:
        return False


def can_import2(kind: 'ShaderKind', imp_mod: tp.Type['ShaderModule']) -> bool:

    if kind == 'AnyShader':
        return True
    elif kind == imp_mod.__kind__:
        return True
    else:
        return False


def can_import3(kind: 'ShaderKind', imp_kind: 'ShaderKind') -> bool:

    if imp_kind == 'AnyShader':
        return True
    elif kind == imp_kind:
        return True
    else:
        return False


def is_shader_module(obj: tp.Any) -> bool:
    # [TODO] issubclass()?
    return hasattr(obj, '__glsl_version__')


def has_params_qualifier(tp_name: str) -> bool:
    return tp_name.startswith('in ') or tp_name.startswith('out ') or tp_name.startswith('inout ')


def get_stmt_indent(node: ast.AST, ctx: CompilerContext) -> str:
    return (node.col_offset - ctx.top_indent) * ' ' if ctx.config['pretty_compile'] else ''


def visit_all(node: ast.AST):
    class Visitor(ast.NodeVisitor):
        def visit(self, node):
            print(node)
            super().visit(node)

    Visitor().visit(node)


def dump_ast(node: ast.AST) -> str:
    return ast.dump(node, indent=2)
