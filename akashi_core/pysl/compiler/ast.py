# pyright: reportPrivateUsage=false
from __future__ import annotations

from akashi_core.pysl import _gl
from .items import CompileError, CompilerConfig, CompilerContext, _TGLSL
from . import utils as compiler_utils
from . import transformer
from .symbol import global_symbol_analysis, class_symbol_analysis

from typing import Callable, Protocol, cast, Type, Union, TypeVar, Any, Optional, TypedDict
import typing as tp
from dataclasses import dataclass, field
import inspect
import ast
import pydoc

if tp.TYPE_CHECKING:
    from akashi_core.pysl.shader import ShaderModule


@dataclass
class NodeOut:
    node: ast.AST
    content: str


''' Statements '''


@dataclass
class stmtOut(NodeOut):
    node: ast.stmt
    content: str


@dataclass
class FunctionDefOut(stmtOut):
    node: ast.FunctionDef
    content: str
    is_shader_func: bool = True


def from_FunctionDef(node: ast.FunctionDef, ctx: CompilerContext, cls_name: str = '') -> FunctionDefOut:

    is_method = False
    # [TODO] more efficient way?
    if len(node.decorator_list) == 0:
        return FunctionDefOut(node, '', False)
    for deco in node.decorator_list:
        deco_node = compile_expr(deco, ctx)
        deco_tpname = transformer.type_transformer(deco_node.content, ctx, deco_node.node)
        if deco_tpname not in ['method', 'func', 'test_func']:
            return FunctionDefOut(node, '', False)
        if deco_tpname == 'method':
            is_method = True

    func_name = compiler_utils.mangle_shader_func(cls_name, node.name) if len(cls_name) > 0 else node.name

    ctx.top_indent = node.col_offset

    if not node.returns:
        raise CompileError('Return type annotation not found')

    returns = compile_expr(node.returns, ctx)
    returns_str = transformer.type_transformer(returns.content, ctx, returns.node)

    if compiler_utils.has_params_qualifier(returns_str):
        raise CompileError('Return type must not have its parameter qualifier')

    args = from_arguments(node.args, ctx, 1 if is_method else 0)
    args_temp = []
    for idx, arg in enumerate(args):
        arg_tpname = transformer.type_transformer(arg.content, ctx, arg.node)
        ctx.symbol[arg.node.arg] = arg_tpname
        args_temp.append(f'{arg_tpname} {arg.node.arg}')

    args_str = ', '.join(args_temp)

    # [XXX] Now we know the types of arguments, and can utilize them for body compilation
    body_str = transformer.body_transformer(node.body, ctx, brace_on_ellipsis=False)

    content = f'{returns_str} {func_name}({args_str}){body_str}'

    imported_strs = []
    for imp in ctx.imported_current.values():
        if not ctx.shmod_klass:
            # [TODO] better msg?
            raise CompileError('ctx.shmod_klass is null')
        elif not compiler_utils.can_import(ctx.shmod_klass, imp[0]):
            raise CompileError(f'Forbidden import {imp[0].__kind__} from {ctx.shmod_klass.__kind__}')

        imported_strs.append(compile_shader_staticmethod(imp[0], imp[1], True, ctx.config))

    content = "".join(imported_strs) + content

    return FunctionDefOut(node, content)


@dataclass
class ClassDefOut(stmtOut):
    node: ast.ClassDef
    content: str


def from_ClassDef(node: ast.ClassDef, ctx: CompilerContext) -> ClassDefOut:

    body_strs = []
    for stmt in node.body:
        if isinstance(stmt, ast.FunctionDef):
            out = from_FunctionDef(stmt, ctx, node.name)
            if out.is_shader_func:
                body_strs.append(out.content)
            ctx.imported_current.clear()

    body_joiner = '\n\n' if ctx.config['pretty_compile'] else ''
    body_str = body_joiner.join(body_strs)

    content = ''
    if body_str == '...':
        content = ''
    else:
        content = body_str

    return ClassDefOut(node, content)


@dataclass
class ReturnOut(stmtOut):
    node: ast.Return
    content: str


def from_Return(node: ast.Return, ctx: CompilerContext) -> ReturnOut:

    content = 'return;'
    if node.value:
        out = compile_expr(node.value, ctx)
        if out.content != "None":
            content = f'return {out.content};'

    content = compiler_utils.get_stmt_indent(node, ctx) + content
    return ReturnOut(node, content)


@dataclass
class AssignOut(stmtOut):
    node: ast.Assign
    content: str


def from_Assign(node: ast.Assign, ctx: CompilerContext) -> AssignOut:
    ''' '''

    ''' Multiple Assignment
    Currently, we forbid the Multiple Assignment in pyshader codes for the following reasons.
    1. Difficulty to distinguish unwanted unpacking from multiple assignments
    At least on the AST, multiple assignments have two forms: normal multiple assignment, and unpacking.
    Normal multiple assignment, for example, is such statements like `a = b = 1`, and unpacking is like
    `a, b = (1, 2)`. Unpacking is clearly not compilable in GLSL, so we need to avoid that one. However,
    both assignments are hard to be distinguished on AST. Perhaps, you may think that we can do it depending
    on whether `ast.Assign.value` is tuple/list or not. But, think about an expression like `a, b = func()`.
    In such cases where the `ast.Assign.value` is `ast.Call`, we will immediately lose our basis for judgment.
    We cannot determine the return type of `ast.Call` on the AST. In order to solve these issues, we have to
    tackle on numerous messy stuffs like symbol resolution. That must be a relatively arduous task for us.
    2. Not so much profit, and besides we have another option
    In my opinion, multiple assignment, especially normal multiple assignment, does make not much profit,
    and are not widely used in Python. In addition, we have another option for this; we can just use single
    assignment. We will not have any troubles without multiple assignment, after all.
    '''

    ''' Declaration Assignment
    In other languages like C/C++/GLGL, declaration and definition are strictly distinguished.
    But in Python, the distinction between both is ambiguous. So, an expression like `a = 1` can be interpreted as
    both simple assignment and declaration assignment, by which we mean the assignment where a declaration and
    an assignment for a certain variable are executed in a single statement.
    However, in GLSL, no-typed declaration assignment is invalid. So, in such cases, it is generally not possible to
    compile declaration assignment in Python correctly without resorting to ctx resolution or other means.
    As for now, we forbid no-typed declaration assignment in pyshader.
    '''

    targets_str = ' = '.join([compile_expr(target, ctx).content for target in node.targets])
    value_str = compile_expr(node.value, ctx).content

    content = compiler_utils.get_stmt_indent(node, ctx) + f'{targets_str} = {value_str};'

    return AssignOut(node, content)


@dataclass
class AugAssignOut(stmtOut):
    node: ast.AugAssign
    content: str


def from_AugAssign(node: ast.AugAssign, ctx: CompilerContext) -> AugAssignOut:

    target_str = compile_expr(node.target, ctx).content
    op_str = transformer.binop_transformer(node.op)
    value_str = compile_expr(node.value, ctx).content

    content = compiler_utils.get_stmt_indent(node, ctx) + f'{target_str} {op_str}= {value_str};'

    return AugAssignOut(node, content)


@dataclass
class AnnAssignOut(stmtOut):
    node: ast.AnnAssign
    content: str


def from_AnnAssign(node: ast.AnnAssign, ctx: CompilerContext) -> AnnAssignOut:

    if not node.simple:
        # For the term `simple`, see https://docs.python.org/ja/3/library/ast.html#ast.AnnAssign
        raise CompileError('Non simple Assignment is forbidden')

    target_str = compile_expr(node.target, ctx).content
    ann_str = from_annotation(node.annotation, ctx)

    content = f'{ann_str} {target_str};'

    if node.value:
        value_str = compile_expr(node.value, ctx).content
        content = f'{ann_str} {target_str} = {value_str};'

    content = compiler_utils.get_stmt_indent(node, ctx) + content

    return AnnAssignOut(node, content)


@dataclass
class ForOut(stmtOut):
    node: ast.For
    content: str


def from_For(node: ast.For, ctx: CompilerContext) -> ForOut:

    iter_out = compile_expr(node.iter, ctx)
    if not isinstance(iter_out, CallOut) or not iter_out.content.startswith('range('):
        raise CompileError(
            'On For statement, only a format like `for idx in range(start,stop,step)` is currently supported.'
        )

    if len(node.orelse) > 0:
        raise CompileError('Else with For is not supported')

    idx_length = 0
    idx_start = 0
    idx_step = 1
    if len(iter_out.args) == 1:
        idx_length = int(iter_out.args[0])
    elif len(iter_out.args) == 2:
        idx_start = int(iter_out.args[0])
        idx_length = int(iter_out.args[1])
    elif len(iter_out.args) == 3:
        idx_start = int(iter_out.args[0])
        idx_length = int(iter_out.args[1])
        idx_step = int(iter_out.args[2])

    idx = compile_expr(node.target, ctx).content

    body_str = transformer.body_transformer(node.body, ctx)

    content = compiler_utils.get_stmt_indent(node, ctx) + \
        f'for(int {idx}=({idx_start});{idx}<({idx_length});{idx}+=({idx_step})){body_str}'

    return ForOut(node, content)


@dataclass
class WhileOut(stmtOut):
    node: ast.While
    content: str


def from_While(node: ast.While, ctx: CompilerContext) -> WhileOut:

    if len(node.orelse) > 0:
        raise CompileError('Else with while is not supported')

    test_str = compile_expr(node.test, ctx).content
    body_str = transformer.body_transformer(node.body, ctx)

    content = compiler_utils.get_stmt_indent(node, ctx) + f'while({test_str}){body_str}'

    return WhileOut(node, content)


@dataclass
class IfOut(stmtOut):
    node: ast.If
    content: str


def from_If(node: ast.If, ctx: CompilerContext) -> IfOut:

    test_str = compile_expr(node.test, ctx).content
    body_str = transformer.body_transformer(node.body, ctx)

    content = f'if({test_str}){body_str}'

    if len(node.orelse) > 0:
        else_str = transformer.body_transformer(node.orelse, ctx)
        content += f'else{else_str}'

    content = compiler_utils.get_stmt_indent(node, ctx) + content

    return IfOut(node, content)


@dataclass
class ExprOut(stmtOut):
    node: ast.Expr
    content: str


def from_Expr(node: ast.Expr, ctx: CompilerContext) -> ExprOut:

    out = compile_expr(node.value, ctx)
    last_str = ';' if out.content != '...' else ''
    content = compiler_utils.get_stmt_indent(node, ctx) + out.content + last_str

    return ExprOut(node, content)


@dataclass
class BreakOut(stmtOut):
    node: ast.Break
    content: str


def from_Break(node: ast.Break, ctx: CompilerContext) -> BreakOut:

    content = compiler_utils.get_stmt_indent(node, ctx) + 'break;'
    return BreakOut(node, content)


@dataclass
class ContinueOut(stmtOut):
    node: ast.Continue
    content: str


def from_Continue(node: ast.Continue, ctx: CompilerContext) -> ContinueOut:

    content = compiler_utils.get_stmt_indent(node, ctx) + 'continue;'
    return ContinueOut(node, content)


''' Expressions '''


@dataclass
class exprOut(NodeOut):
    node: ast.expr
    content: str


@dataclass
class BoolOpOut(exprOut):
    node: ast.BoolOp
    content: str


def from_BoolOp(node: ast.BoolOp, ctx: CompilerContext) -> BoolOpOut:

    op_str = transformer.boolop_transformer(node.op)

    value_strs = []
    for value in node.values:
        value_strs.append(f'({compile_expr(value, ctx).content})')

    content = f' {op_str} '.join(value_strs)
    return BoolOpOut(node, content)


@dataclass
class UnaryOpOut(exprOut):
    node: ast.UnaryOp
    content: str


def from_UnaryOp(node: ast.UnaryOp, ctx: CompilerContext) -> UnaryOpOut:

    op_str = transformer.unaryop_transformer(node.op)
    operand_str = compile_expr(node.operand, ctx).content
    content = f'{op_str}{operand_str}'

    return UnaryOpOut(node, content)


@dataclass
class BinOpOut(exprOut):
    node: ast.BinOp
    content: str


def from_BinOp(node: ast.BinOp, ctx: CompilerContext) -> BinOpOut:

    op_str = transformer.binop_transformer(node.op)

    # 1. All non single terms are parenthesized.
    # left_str = compile_expr(node.left).content
    # if not isinstance(node.left, ast.Constant) and not isinstance(node.left, ast.Name):
    #     left_str = '(' + left_str + ')'
    # right_str = compile_expr(node.right).content
    # if not isinstance(node.right, ast.Constant) and not isinstance(node.right, ast.Name):
    #     right_str = '(' + right_str + ')'
    # content = f'{left_str} {op_str} {right_str}'

    # 2. All terms are parenthesized.
    left_str = compile_expr(node.left, ctx).content
    right_str = compile_expr(node.right, ctx).content
    content = f'({left_str}) {op_str} ({right_str})'

    return BinOpOut(node, content)


@dataclass
class IfExpOut(exprOut):
    node: ast.IfExp
    content: str


def from_IfExp(node: ast.IfExp, ctx: CompilerContext) -> IfExpOut:

    test_str = compile_expr(node.test, ctx).content
    body_str = compile_expr(node.body, ctx).content
    else_str = compile_expr(node.orelse, ctx).content

    content = f'{test_str} ? {body_str} : {else_str}'

    return IfExpOut(node, content)


@dataclass
class CompareOut(exprOut):
    node: ast.Compare
    content: str


def from_Compare(node: ast.Compare, ctx: CompilerContext) -> CompareOut:

    if len(node.ops) > 1 or len(node.comparators) > 1:
        raise CompileError((
            'Multiple comparison operators in one expression is not supported. ' +
            'Consider split the expression like `1 <= a < 10` ==> `1 <= a and a < 10`.'
        ))

    op_str = transformer.cmpop_transformer(node.ops[0])

    left_str = compile_expr(node.left, ctx).content
    right_str = compile_expr(node.comparators[0], ctx).content
    content = f'({left_str}) {op_str} ({right_str})'

    return CompareOut(node, content)


@dataclass
class CallOut(exprOut):
    node: ast.Call
    args: list[str]
    content: str


def from_Call(node: ast.Call, ctx: CompilerContext) -> CallOut:

    func_str = compile_expr(node.func, ctx).content
    args = [compile_expr(arg, ctx).content for arg in node.args]
    args_str = ', '.join(args)

    # [TODO] maybe we should explicitly forbid the use of keyword args, *args, **kwargs

    content = f'{func_str}({args_str})'

    return CallOut(node, args=args, content=content)


@dataclass
class ConstantOut(exprOut):
    node: ast.Constant
    content: str


def from_Constant(node: ast.Constant, ctx: CompilerContext) -> ConstantOut:
    content = ''
    if node.value == Ellipsis:
        content = '...'
    elif isinstance(node.value, bool) and node.value == True:  # noqa: E712
        content = 'true'
    elif isinstance(node.value, bool) and node.value == False:  # noqa: E712
        content = 'false'
    else:
        content = str(node.value)

    return ConstantOut(node, content)


@dataclass
class AttributeOut(exprOut):
    node: ast.Attribute
    content: str

# [TODO] needs refactoring


def from_Attribute(node: ast.Attribute, ctx: CompilerContext) -> AttributeOut:

    value_str = compile_expr(node.value, ctx).content
    attr_str = str(node.attr)

    # module global
    if value_str in ctx.global_symbol:

        # [TODO] maybe we need to refactor these lines
        if inspect.ismodule(ctx.global_symbol[value_str]):
            if (m := getattr(ctx.global_symbol[value_str], attr_str)) and compiler_utils.is_shader_module(m):
                ctx.global_symbol[attr_str] = m
            content = f'{attr_str}'
            return AttributeOut(node, content)

        skip_global_resolution = not(ctx.on_import_resolution) and value_str == ctx.shmod_name
        if not(skip_global_resolution) and compiler_utils.is_shader_module(ctx.global_symbol[value_str]):
            shader_func = getattr(ctx.global_symbol[value_str], attr_str)
            if shader_func not in ctx.imported:
                ctx.imported.add(shader_func)
                ctx.imported_current[shader_func] = (ctx.global_symbol[value_str], shader_func)
            content = compiler_utils.mangle_shader_func(value_str, attr_str)
            return AttributeOut(node, content)
        elif value_str == 'gl':
            if hasattr(_gl, attr_str):
                content = f'{attr_str}'
                return AttributeOut(node, content)

    # method/class global
    if value_str in ctx.symbol:
        value_tpname: str = ctx.symbol[value_str]
        if compiler_utils.has_params_qualifier(value_tpname):
            # perhaps this cond always holds
            if attr_str == 'value':
                attr_str = ''
                content = f'{value_str}'
                return AttributeOut(node, content)
    if value_str == 'self':
        if attr_str in ctx.cls_symbol and ctx.cls_symbol[attr_str][0] == 'instancemethod' and ctx.shmod_name:
            content = compiler_utils.mangle_shader_func(ctx.shmod_name, attr_str)
            return AttributeOut(node, content)
        else:
            content = f'{attr_str}'
            return AttributeOut(node, content)

    if value_str in ctx.cls_symbol:
        value_tpname: str = ctx.cls_symbol[value_str][0]
        if value_tpname.startswith('uniform'):
            if attr_str == 'value':
                content = f'{value_str}'
                return AttributeOut(node, content)
        elif value_tpname.startswith('dynamic'):
            if attr_str == 'value' and ctx.shmod_inst:
                content = str(getattr(getattr(ctx.shmod_inst, value_str), 'value'))
                return AttributeOut(node, content)
        elif value_tpname.startswith('out_t'):
            if attr_str == 'value':
                content = f'{value_str}'
                return AttributeOut(node, content)
        elif value_tpname.startswith('in_t'):
            if attr_str == 'value':
                content = f'{value_str}'
                return AttributeOut(node, content)

    # [TODO] change this impl later for symbol resolution based one
    if ctx.shmod_name and value_str == ctx.shmod_name:
        content = compiler_utils.mangle_shader_func(value_str, attr_str)
        return AttributeOut(node, content)

    content = f'{value_str}.{attr_str}'
    return AttributeOut(node, content)


@dataclass
class SubscriptOut(exprOut):
    node: ast.Subscript
    content: str


def from_Subscript(node: ast.Subscript, ctx: CompilerContext) -> SubscriptOut:

    value_str = compile_expr(node.value, ctx).content
    slice_str = compile_expr(node.slice, ctx).content

    content = f'{value_str}[{slice_str}]'

    return SubscriptOut(node, content)


@dataclass
class NameOut(exprOut):
    node: ast.Name
    content: str


def from_Name(node: ast.Name, ctx: CompilerContext) -> NameOut:
    content = ''
    if not hasattr(node, 'id'):
        content = 'None'
    else:
        name_str = str(node.id)
        if name_str in ctx.local_symbol:
            content = f'{ctx.local_symbol[name_str]}'
        elif name_str in ctx.global_symbol and type(ctx.global_symbol[name_str]) in [int, float]:
            content = f'{ctx.global_symbol[name_str]}'
        else:
            content = str(node.id)
    return NameOut(node, content)


@dataclass
class ListOut(exprOut):
    node: ast.List
    content: str


def from_List(node: ast.List, ctx: CompilerContext) -> ListOut:

    elts_str = ', '.join([compile_expr(elt, ctx).content for elt in node.elts])
    content = f'[{elts_str}]'

    return ListOut(node, content)


@dataclass
class TupleOut(exprOut):
    node: ast.Tuple
    content: str


def from_Tuple(node: ast.Tuple, ctx: CompilerContext) -> TupleOut:

    elts_str = ', '.join([compile_expr(elt, ctx).content for elt in node.elts])
    content = f'({elts_str})'

    return TupleOut(node, content)


@dataclass
class SliceOut(exprOut):
    node: ast.Slice
    content: str


def from_Slice(node: ast.Slice, ctx: CompilerContext) -> SliceOut:

    lower_str = compile_expr(node.lower, ctx).content if node.lower else ''
    upper_str = compile_expr(node.upper, ctx).content if node.upper else ''
    step_str = compile_expr(node.step, ctx).content if node.step else ''

    content = f'{lower_str}:{upper_str}'
    if step_str != '':
        content += f':{step_str}'

    return SliceOut(node, content)


''' Function Arguments '''


def from_arguments(node: ast.arguments, ctx: CompilerContext, args_offset: int = 0) -> list[argOut]:
    # posonlyargs, kwonlyargs, vararg, kwargs, kw_defaults, defaults are just ignored for now
    return [from_arg(ag, ctx) for ag in node.args[args_offset:]]


@dataclass
class argOut(NodeOut):
    node: ast.arg
    content: str


def from_arg(node: ast.arg, ctx: CompilerContext) -> argOut:
    if not node.annotation:
        raise CompileError('All arguments must be explicitly type-hinted!')
    content = from_annotation(node.annotation, ctx)
    return argOut(node, content)


''' Others '''


def from_annotation(node: Union[ast.Name, ast.Constant, ast.Attribute, ast.expr], ctx: CompilerContext) -> str:
    tp_name: str = ''
    if isinstance(node, ast.Name):
        tp_name = from_Name(node, ctx).content
    elif isinstance(node, ast.Constant):
        tp_name = str(from_Constant(node, ctx))
    elif isinstance(node, ast.Attribute):
        tp_name = from_Attribute(node, ctx).content
    else:
        # raise CompileError(f'Unexpected node type `{type(node)}` found')
        tp_name = compile_expr(node, ctx).content

    return tp_name

    # # https://stackoverflow.com/a/29831586
    # basic_tp = pydoc.locate(tp_name)
    # if basic_tp:
    #     return cast(Type, basic_tp)
    # raise CompileError(f'from_annotation(): Unknown tp_name `{tp_name}` found')


def any_instance(node: ast.AST, node_types: tuple[tp.Type[ast.AST], ...]) -> bool:

    for node_type in node_types:
        if isinstance(node, node_type):
            return True
    else:
        return False


def compile_stmt(node: ast.stmt, ctx: CompilerContext) -> stmtOut:

    never_types = (
        ast.AsyncFunctionDef,
        ast.Delete,
        ast.AsyncFor,
        ast.With,
        ast.AsyncWith,
        ast.Raise,
        ast.Try,
        ast.Assert,
        ast.Import,
        ast.ImportFrom,
        ast.Global,
        ast.Nonlocal,
        ast.Pass)

    if isinstance(node, ast.FunctionDef):
        return from_FunctionDef(node, ctx)
    if isinstance(node, ast.Return):
        return from_Return(node, ctx)
    if isinstance(node, ast.Assign):
        return from_Assign(node, ctx)
    if isinstance(node, ast.AugAssign):
        return from_AugAssign(node, ctx)
    if isinstance(node, ast.AnnAssign):
        return from_AnnAssign(node, ctx)
    if isinstance(node, ast.For):
        return from_For(node, ctx)
    if isinstance(node, ast.While):
        return from_While(node, ctx)
    if isinstance(node, ast.If):
        return from_If(node, ctx)
    if isinstance(node, ast.Expr):
        return from_Expr(node, ctx)
    if isinstance(node, ast.Break):
        return from_Break(node, ctx)
    if isinstance(node, ast.Continue):
        return from_Continue(node, ctx)

    if any_instance(node, never_types):
        raise CompileError(f'The statement `{type(node)}` is forbidden')

    raise CompileError(f'Not implemented statement `{type(node)}` found')


def compile_expr(node: ast.expr, ctx: CompilerContext) -> exprOut:

    never_types = (
        ast.Lambda,
        ast.Dict,
        ast.Set,
        ast.ListComp,
        ast.SetComp,
        ast.DictComp,
        ast.GeneratorExp,
        ast.Await,
        ast.Yield,
        ast.YieldFrom,
        ast.Starred)

    if isinstance(node, ast.BoolOp):
        return from_BoolOp(node, ctx)
    if isinstance(node, ast.BinOp):
        return from_BinOp(node, ctx)
    if isinstance(node, ast.UnaryOp):
        return from_UnaryOp(node, ctx)
    if isinstance(node, ast.IfExp):
        return from_IfExp(node, ctx)
    if isinstance(node, ast.Compare):
        return from_Compare(node, ctx)
    if isinstance(node, ast.Call):
        return from_Call(node, ctx)
    if isinstance(node, ast.Constant):
        return from_Constant(node, ctx)
    if isinstance(node, ast.Attribute):
        return from_Attribute(node, ctx)
    if isinstance(node, ast.Subscript):
        return from_Subscript(node, ctx)
    if isinstance(node, ast.Name):
        return from_Name(node, ctx)
    if isinstance(node, ast.List):
        return from_List(node, ctx)
    if isinstance(node, ast.Tuple):
        return from_Tuple(node, ctx)
    if isinstance(node, ast.Slice):
        return from_Slice(node, ctx)

    if any_instance(node, (ast.FormattedValue, ast.JoinedStr)):
        raise CompileError('f-string is not supported as for now')

    if any_instance(node, never_types):
        raise CompileError(f'The expression `{type(node)}` is forbidden')

    raise CompileError(f'Not implemented expression `{type(node)}` found')


def compile_shader_staticmethod(
        klass: tp.Type['ShaderModule'],
        method: tp.Callable,
        on_import_resolution: bool,
        config: CompilerConfig.Config = CompilerConfig.default()) -> _TGLSL:

    py_src = compiler_utils.get_source(method)
    root = ast.parse(py_src)
    func_def = compiler_utils.get_function_def(root)

    ctx = CompilerContext(config)
    ctx.on_import_resolution = on_import_resolution

    ctx.shmod_name = klass.__name__
    ctx.shmod_klass = klass
    global_symbol_analysis(klass, ctx)
    class_symbol_analysis(klass, ctx)

    out = from_FunctionDef(func_def, ctx, ctx.shmod_name)

    return out.content
