from __future__ import annotations

from .items import CompilerConfig, CompileError, CompilerContext
from .ast import compile_expr, compile_stmt

import typing as tp
import ast


def __reduce_type_name(org_tp_name: str, res_tp_name: str,
                       ctx: CompilerContext, node: tp.Union[ast.Attribute, ast.Subscript, ast.expr]) -> str:
    # [TODO] needs refactoring
    if isinstance(node, ast.Attribute):
        return res_tp_name + node.attr
    if isinstance(node, ast.Subscript):
        value_str = __reduce_type_name(org_tp_name, '', ctx, node.value)
        if value_str in ['in_p', 'out_p', 'inout_p']:
            res_tp_name += value_str[:-2] + ' '
            _node = compile_expr(node.slice, ctx).node
            return __reduce_type_name(org_tp_name, res_tp_name, ctx, _node)
        else:
            raise CompileError(f'Invalid generic type `{org_tp_name}` found')

    if isinstance(node, ast.Call):
        value_str = compile_expr(node.func, ctx).content
        return res_tp_name + value_str

    return res_tp_name + compile_expr(node, ctx).content


def type_transformer(tp_name: str, ctx: CompilerContext, node: tp.Optional[ast.AST] = None) -> str:

    __tp_name = tp_name
    if node and isinstance(node, ast.arg):
        if node.annotation:
            __tp_name = __reduce_type_name(tp_name, '', ctx, node.annotation)
    elif node and isinstance(node, ast.expr):
        __tp_name = __reduce_type_name(tp_name, '', ctx, node)

    if __tp_name == 'None':
        return 'void'

    # [TODO] impl later
    return __tp_name


def boolop_transformer(op: ast.boolop) -> str:

    if isinstance(op, ast.And):
        return '&&'
    elif isinstance(op, ast.Or):
        return '||'
    else:
        raise CompileError(f'Unexpected operator `{op}` found')


def unaryop_transformer(op: ast.unaryop) -> str:

    if isinstance(op, ast.UAdd):
        return '+'
    elif isinstance(op, ast.USub):
        return '-'
    elif isinstance(op, ast.Not):
        return '!'
    elif isinstance(op, ast.Invert):
        return '~'
    else:
        raise CompileError(f'Unexpected operator `{op}` found')


def binop_transformer(op: ast.operator) -> str:

    if isinstance(op, ast.Add):
        return '+'
    elif isinstance(op, ast.Sub):
        return '-'
    elif isinstance(op, ast.Mult):
        return '*'
    elif isinstance(op, ast.Div):
        return '/'
    elif isinstance(op, ast.FloorDiv):
        raise CompileError('floordiv operator `//` not supported')
    elif isinstance(op, ast.Mod):
        return '%'
    elif isinstance(op, ast.Pow):
        raise CompileError('pow operator `**` not supported')
    elif isinstance(op, ast.LShift):
        return '<<'
    elif isinstance(op, ast.RShift):
        return '>>'
    elif isinstance(op, ast.BitOr):
        return '|'
    elif isinstance(op, ast.BitXor):
        return '^'
    elif isinstance(op, ast.BitAnd):
        return '&'
    elif isinstance(op, ast.MatMult):
        raise CompileError('matmul operator `@` not supported')
    else:
        raise CompileError(f'Unexpected operator `{op}` found')


def cmpop_transformer(op: ast.cmpop) -> str:
    if isinstance(op, ast.Eq):
        return '=='
    elif isinstance(op, ast.NotEq):
        return '!='
    elif isinstance(op, ast.Lt):
        return '<'
    elif isinstance(op, ast.LtE):
        return '<='
    elif isinstance(op, ast.Gt):
        return '>'
    elif isinstance(op, ast.GtE):
        return '>='
    else:
        raise CompileError(f'Operator `{op}` is not supported')


def body_transformer(body: list[ast.stmt], ctx: CompilerContext, brace_on_ellipsis: bool = True) -> str:

    body_strs = []
    for stmt in body:
        out = compile_stmt(stmt, ctx)
        body_strs.append(out.content)

    body_str = ''
    if len(body_strs) == 1 and body_strs[0].strip() == '...':
        body_str = '{;}' if brace_on_ellipsis else ';'
    else:
        if ctx.config['pretty_compile']:
            body_str = '{\n' + '\n'.join(body_strs) + '\n}'
        else:
            body_str = '{' + ''.join(body_strs) + '}'

    return body_str
