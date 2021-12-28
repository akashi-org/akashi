# pyright: reportPrivateUsage=false
from __future__ import annotations
import typing as tp

from .items import CompilerConfig, CompileError, CompilerContext, _TGLSL
from .ast import compile_expr, from_annotation
from .utils import can_import, entry_point, mangled_func_name, get_shader_kind_from_buffer
from .symbol import collect_global_symbols, collect_local_symbols
from akashi_core.pysl import _gl


import inspect
import ast
import sys

if tp.TYPE_CHECKING:
    from akashi_core.pysl.shader import ShaderCompiler, ShaderKind


def parse_lambda_expr(fn: tp.Callable, ctx: CompilerContext) -> ast.expr:

    if fn.__name__ != '<lambda>':
        raise CompileError('No lambda found in lambda shader')
    else:
        raw_src = inspect.getsource(fn).strip()
        if raw_src.startswith('.'):
            raw_src = raw_src[1:]
        root = ast.parse(raw_src.strip())
        local_ctx = {'content': None}

        class Visitor(ast.NodeVisitor):

            def visit_Lambda(self, node: ast.Lambda):
                if len(node.args.args) != 3:
                    super().visit(node.body)
                else:
                    local_ctx['content'] = node.body  # type: ignore

        Visitor().visit(root)

        if not local_ctx['content']:
            raise CompileError('No lambda found in lambda shader')

        return local_ctx['content']


def maybe_let_expr(node: ast.Call) -> bool:

    # complete case: e(...).tp(...)
    if isinstance(node.func, ast.Attribute) and isinstance(node.func.value, ast.Call):
        return True
    # incomplete case: e(...)
    elif len(node.args) == 1 and isinstance(node.args[0], ast.NamedExpr):
        # [TODO] uncomment below if type inference for let expr is implemented
        # return True
        raise CompileError('Let expression must be explicitly type-hinted by tp()')
    else:
        return False


def compile_lambda_expr(node: ast.expr, ctx: CompilerContext) -> list[str]:

    match node:
        case ast.Call():
            if maybe_let_expr(node):
                return [compile_let_expr(node, ctx)]
            else:
                return [compile_expr(node.args[0], ctx).content]
        case ast.BinOp():
            lhs = compile_lambda_expr(node.left, ctx)
            rhs = compile_lambda_expr(node.right, ctx)
            if isinstance(node.op, ast.BitOr):
                return lhs + rhs
            elif isinstance(node.op, ast.LShift):
                assert(len(lhs) == 1)
                assert(len(rhs) == 1)
                return [lhs[0] + ' = ' + rhs[0]]
            else:
                raise CompileError('All gl.exprs must be concated with `|` or `<<`')
        case _:
            raise CompileError('parse_expr() failed')


def compile_let_expr(node: ast.Call, ctx: CompilerContext) -> str:

    if not isinstance(node.func, ast.Attribute) or not isinstance(node.func.value, ast.Call):
        raise CompileError('Invalid format found in let expression')

    if (named_expr := node.func.value.args[0]) and not isinstance(named_expr, ast.NamedExpr):
        raise CompileError('Let expression accepts only assignment expression as its argument.')

    lhs = compile_expr(named_expr.target, ctx).content
    rhs = compile_expr(named_expr.value, ctx).content

    type_str = from_annotation(node.args[0], ctx)

    return f'{type_str} {lhs} = {rhs}'


def collect_argument_symbols(ctx: CompilerContext, fn: tp.Callable, kind: 'ShaderKind'):

    _, buf_arg, var_arg = inspect.getfullargspec(fn).args
    ctx.lambda_args[buf_arg] = ''
    match kind:
        case 'FragShader':
            if var_arg != 'color':
                ctx.lambda_args[var_arg] = 'color'
        case 'PolygonShader':
            if var_arg != 'pos':
                ctx.lambda_args[var_arg] = 'pos'
        case _:
            raise CompileError(f'Invalid kind {kind} found')


def compile_lambda_shader_partial(
        fn: tp.Callable,
        buffer_type: tp.Type[_gl._buffer_type],
        ctx: CompilerContext) -> tuple[_TGLSL, list[tp.Callable]]:

    kind = get_shader_kind_from_buffer(buffer_type)

    collect_global_symbols(ctx, fn)
    collect_argument_symbols(ctx, fn, kind)
    imported_named_shader_fns = collect_local_symbols(ctx, fn)

    expr_node = parse_lambda_expr(fn, ctx)
    stmts = compile_lambda_expr(expr_node, ctx)
    func_body = ''.join([stmt + ';' for stmt in stmts])

    return (func_body, imported_named_shader_fns)
