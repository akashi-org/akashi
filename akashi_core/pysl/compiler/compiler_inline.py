# pyright: reportPrivateUsage=false
from __future__ import annotations
import typing as tp

from akashi_core.pysl._gl_inline import InlineExprKind
from .items import CompilerConfig, CompileError, CompilerContext, _TGLSL
from .ast import compile_expr, from_annotation
from .utils import can_import3
from .symbol import buffer_symbol_analysis

from . import compiler_named

import inspect
import ast
import sys

if tp.TYPE_CHECKING:
    from akashi_core.pysl.shader import ShaderModule, ShaderKind, TEntryFn


class InlineCompileDetectError(CompileError):
    pass


def entry_point(kind: 'ShaderKind', func_body: str, self_postfix: str = '', next_postfix: str = '') -> str:

    if kind == 'FragShader':
        chain_str = '' if len(next_postfix) == 0 else f'frag_main{next_postfix}(color);'
        return f'void frag_main{self_postfix}(inout vec4 color)' + '{' + func_body + chain_str + '}'
    elif kind == 'PolygonShader':
        chain_str = '' if len(next_postfix) == 0 else f'poly_main{next_postfix}(pos);'
        return f'void poly_main{self_postfix}(inout vec3 pos)' + '{' + func_body + chain_str + '}'
    else:
        raise NotImplementedError()


def get_inline_source(fn: 'TEntryFn', ctx: CompilerContext) -> ast.expr:

    # [XXX] This function is really buggy. But, the code itself is not the culprit.
    # The culprit is the current spec or environments surrounded with the spec.

    if fn.__name__ != '<lambda>':
        raise CompileError('As for inline pysl, we currently support lambda function only!')
    else:
        raw_src = inspect.getsource(fn).strip()
        if raw_src.startswith('.'):
            raw_src = raw_src[1:]
        root = ast.parse(raw_src.strip())
        local_ctx = {'content': None}

        class Visitor(ast.NodeVisitor):

            def visit_Lambda(self, node: ast.Lambda):
                if len(node.args.args) != 2:
                    super().visit(node.body)

                exprs = split_exprs(node.body)
                try:
                    [detect_expr(e, ctx) for e in exprs]
                except InlineCompileDetectError:
                    super().visit(node.body)
                else:
                    local_ctx['content'] = node.body  # type: ignore

        Visitor().visit(root)

        if not local_ctx['content']:
            raise CompileError('No lambda found in inline shader')

        return local_ctx['content']


def split_exprs(expr_node: ast.expr) -> list[ast.Call]:
    '''
        from: gl.expr(...) >> gl.let(...) >> ... >> gl.expr(...)
        to:  [gl.expr(...), gl.let(...), ..., gl.expr(...)]
    '''
    if isinstance(expr_node, ast.Call):
        return [expr_node]
    elif isinstance(expr_node, ast.BinOp):
        if not isinstance(expr_node.op, ast.RShift):
            raise CompileError('All gl.exprs must be concated with `>>`')
        left_strs = split_exprs(expr_node.left)
        right_strs = split_exprs(expr_node.right)
        return left_strs + right_strs
    else:
        raise CompileError('split_exprs() failed')


def detect_expr(raw_expr: ast.Call, ctx: CompilerContext) -> InlineExprKind:
    '''
        input: gl.expr(...) or C(...) (where C is an alias for gl.expr)
        pattern: expr

        input: gl.let(...) or C(...) (where C is an alias for gl.let)
        pattern: let

        input: gl.assign(...) or C(...) (where C is an alias for gl.assign)
        pattern: assign
    '''

    # [TODO] we should resolve the alias here

    # [TODO] any better ways?
    raw_expr_src = ast.unparse(raw_expr)
    head = raw_expr_src.split('(')[0]

    if head.endswith('expr'):
        return 'expr'
    elif head.endswith('assign'):
        return 'assign'
    elif head.endswith('let'):
        return 'let'
    else:
        raise InlineCompileDetectError('Invalid pattern found')


def parse_expr(raw_expr: ast.Call, ctx: CompilerContext) -> str:

    expr_kind = detect_expr(raw_expr, ctx)

    if expr_kind == 'expr':
        return parse_gl_expr(raw_expr, ctx)
    elif expr_kind == 'assign':
        return parse_gl_assign(raw_expr, ctx)
    elif expr_kind == 'let':
        return parse_gl_let(raw_expr, ctx)
    else:
        raise CompileError('parse_expr() failed')


def parse_gl_expr(expr_node: ast.Call, ctx: CompilerContext) -> str:

    return compile_expr(expr_node.args[0], ctx).content + ';'


def parse_gl_assign(assign_node: ast.Call, ctx: CompilerContext) -> str:

    if not isinstance(assign_node.func, ast.Attribute) or not isinstance(assign_node.func.value, ast.Call):
        raise CompileError('Invalid format found in gl.assign')

    lhs = compile_expr(assign_node.func.value.args[0], ctx).content
    op = str(assign_node.func.attr)

    if op == 'eq':
        rhs = compile_expr(assign_node.args[0], ctx).content
        return f'{lhs} = {rhs};'
    elif op == 'op':
        op_str = compile_expr(assign_node.args[0], ctx).content
        rhs = compile_expr(assign_node.args[1], ctx).content
        return f'{lhs} {op_str} {rhs};'
    else:
        return ';'


def parse_gl_let(let_node: ast.Call, ctx: CompilerContext) -> str:

    if not isinstance(let_node.func, ast.Attribute) or not isinstance(let_node.func.value, ast.Call):
        raise CompileError('Invalid format found in gl.let')

    if (named_expr := let_node.func.value.args[0]) and not isinstance(named_expr, ast.NamedExpr):
        raise CompileError('gl.let accepts only assignment expression as its argument.')

    lhs = compile_expr(named_expr.target, ctx).content
    rhs = compile_expr(named_expr.value, ctx).content
    type_str = from_annotation(let_node.args[0], ctx)

    return f'{type_str} {lhs} = {rhs};'


def collect_global_symbols(ctx: CompilerContext, fn: 'TEntryFn'):

    ctx.global_symbol = vars(sys.modules[fn.__module__])


# Used for argument resolution
def collect_instance_symbols(ctx: CompilerContext, sh_mod_fn: tp.Callable[[], 'ShaderModule']):

    buffer_symbol_analysis(sh_mod_fn(), ctx)


def collect_local_symbols(ctx: CompilerContext, fn: 'TEntryFn'):

    for idx, freevar in enumerate(fn.__code__.co_freevars):
        value = fn.__closure__[idx].cell_contents
        ctx.eval_local_symbol[freevar] = value


def collect_argument_symbols(ctx: CompilerContext, fn: 'TEntryFn', kind: 'ShaderKind'):

    buf_arg, var_arg = inspect.getfullargspec(fn).args
    ctx.lambda_args[buf_arg] = ''
    if kind == 'FragShader':
        if var_arg != 'color':
            ctx.lambda_args[var_arg] = 'color'
        ctx.local_symbol['color'] = 'inout vec4'
    elif kind == 'PolygonShader':
        if var_arg != 'pos':
            ctx.lambda_args[var_arg] = 'pos'
        ctx.local_symbol['pos'] = 'inout vec4'


def compile_inline_shaders(
        fns: tuple['TEntryFn', ...],
        sh_mod_fn: tp.Callable[[], 'ShaderModule'],
        config: CompilerConfig.Config = CompilerConfig.default()) -> _TGLSL:

    kind = sh_mod_fn().__kind__

    if len(fns) == 0:
        return entry_point(kind, '')

    stmts = []
    imported_named_shader_fns_dict = {}

    ctx = CompilerContext(config)
    # [TODO] maybe we should call this in the loop below
    collect_global_symbols(ctx, fns[0])
    collect_instance_symbols(ctx, sh_mod_fn)

    for idx, fn in enumerate(fns):

        stmt = []

        expr_node = get_inline_source(fn, ctx)
        exprs = split_exprs(expr_node)

        ctx.lambda_args = {}
        collect_argument_symbols(ctx, fn, kind)
        for imp_fn in compiler_named.collect_local_symbols(ctx, fn):
            imported_named_shader_fns_dict[compiler_named.mangled_func_name(ctx, imp_fn)] = imp_fn

        for expr in exprs:
            stmt.append(parse_expr(expr, ctx))

        self_postfix = f'_{idx}' if idx > 0 else ''
        next_postfix = f'_{idx + 1}' if idx != len(fns) - 1 else ''

        stmts.insert(0, entry_point(kind, ''.join(stmt), self_postfix, next_postfix))

    imported_strs = []
    for imp_fn in imported_named_shader_fns_dict.values():
        imp_shader_kind = compiler_named.to_shader_kind(tp.cast(tuple, inspect.getfullargspec(imp_fn).defaults)[0])
        if not can_import3(kind, imp_shader_kind):
            raise CompileError(f'Forbidden import {imp_shader_kind} from {kind}')
        imported_strs.append(compiler_named.compile_named_shader(imp_fn, ctx.config))

    return "".join(imported_strs) + ''.join(stmts)


def compile_inline_shader_partial(
        fn: 'TEntryFn',
        sh_mod_fn: tp.Callable[[], 'ShaderModule'],
        ctx: CompilerContext) -> tuple[_TGLSL, list[tp.Callable]]:

    kind = sh_mod_fn().__kind__

    collect_global_symbols(ctx, fn)
    collect_instance_symbols(ctx, sh_mod_fn)

    expr_node = get_inline_source(fn, ctx)
    exprs = split_exprs(expr_node)

    collect_argument_symbols(ctx, fn, kind)
    imported_named_shader_fns = compiler_named.collect_local_symbols(ctx, fn)

    stmt = []
    for expr in exprs:
        stmt.append(parse_expr(expr, ctx))

    func_body = ''.join(stmt)

    return (func_body, imported_named_shader_fns)
