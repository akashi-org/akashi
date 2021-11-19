from __future__ import annotations

from . import _gl

from typing import Callable, Protocol, cast, Type, Union, TypeVar, Any, Optional, TypedDict
import typing as tp
from types import ModuleType
from dataclasses import dataclass, field
import inspect
import ast
import sys
import pydoc
import re

if tp.TYPE_CHECKING:
    from .shader import ShaderModule


class CompileError(Exception):
    pass


_TGLSL = str


class __priv:

    class HasModule(Protocol):
        __module__: str

    @staticmethod
    def get_module(obj: HasModule) -> ModuleType:
        return sys.modules[obj.__module__]

    @staticmethod
    def get_source(obj) -> str:
        raw_src = inspect.getsource(obj)

        # raw_src can be indented already, and that will cause IndentationError in ast.parse().
        # So here we explicitly strip the indents.
        lines = raw_src.split('\n')
        top_indent = m.span()[0] if (m := re.search(r'[^\s]', lines[0])) else 0
        return '\n'.join([ln[top_indent:] for ln in lines])

    @staticmethod
    def unwrap_func(fn: Callable) -> Callable:
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
        return cast(ast.FunctionDef, ctx['node'])


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
        return cast(ast.ClassDef, ctx['node'])


def mangle_shader_func(shmod_name: str, func_name: str) -> str:
    if func_name in ['frag_main', 'main', 'poly_main']:
        return func_name
    else:
        return shmod_name + '_' + func_name


def has_params_qualifier(tp_name: str) -> bool:
    return tp_name.startswith('in ') or tp_name.startswith('out ') or tp_name.startswith('inout ')


# [TODO] needs refactoring
def reduce_type_name(org_tp_name: str, res_tp_name: str,
                     ctx: CompilerContext, node: Union[ast.Attribute, ast.Subscript, ast.expr]) -> str:
    if isinstance(node, ast.Attribute):
        return res_tp_name + node.attr
    if isinstance(node, ast.Subscript):

        value_str = reduce_type_name(org_tp_name, '', ctx, node.value)
        if value_str in ['in_p', 'out_p', 'inout_p']:
            res_tp_name += value_str[:-2] + ' '
            _node = compile_expr(node.slice, ctx).node
            return reduce_type_name(org_tp_name, res_tp_name, ctx, _node)
        else:
            raise CompileError(f'Invalid generic type `{org_tp_name}` found')

    return res_tp_name + compile_expr(node, ctx).content


def type_transformer(tp_name: str, ctx: CompilerContext, node: Optional[ast.AST] = None) -> str:

    __tp_name = tp_name
    if node and isinstance(node, ast.arg):
        if node.annotation:
            __tp_name = reduce_type_name(tp_name, '', ctx, node.annotation)
    elif node and isinstance(node, ast.expr):
        __tp_name = reduce_type_name(tp_name, '', ctx, node)

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


_TType = TypeVar('_TType', bound=Type)


class CompilerConfig:

    class Config(TypedDict):
        ''' '''

        ''' If True, compiler tries to output more human-readable shader code'''
        pretty_compile: bool

    @staticmethod
    def default() -> Config:

        return {
            'pretty_compile': False
        }


@dataclass
class CompilerContext:
    config: CompilerConfig.Config
    symbol: dict = field(default_factory=dict)
    cls_symbol: dict = field(default_factory=dict)
    top_indent: int = field(default=0, init=False)
    shmod_name: str = field(default='', init=False)


def get_stmt_indent(node: ast.AST, ctx: CompilerContext) -> str:
    return (node.col_offset - ctx.top_indent) * ' ' if ctx.config['pretty_compile'] else ''


class transformer:

    # SymbolTable = dict

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

    @staticmethod
    def from_FunctionDef(node: ast.FunctionDef, ctx: CompilerContext, cls_name: str = '') -> FunctionDefOut:

        is_method = False
        # [TODO] more efficient way?
        if len(node.decorator_list) == 0:
            return transformer.FunctionDefOut(node, '', False)
        for deco in node.decorator_list:
            deco_node = compile_expr(deco, ctx)
            deco_tpname = type_transformer(deco_node.content, ctx, deco_node.node)
            if deco_tpname not in ['method', 'func', 'test_func']:
                return transformer.FunctionDefOut(node, '', False)
            if deco_tpname == 'method':
                is_method = True

        func_name = mangle_shader_func(cls_name, node.name) if len(cls_name) > 0 else node.name

        ctx.top_indent = node.col_offset

        if not node.returns:
            raise CompileError('Return type annotation not found')

        returns = compile_expr(node.returns, ctx)
        returns_str = type_transformer(returns.content, ctx, returns.node)

        if has_params_qualifier(returns_str):
            raise CompileError('Return type must not have its parameter qualifier')

        args = transformer.from_arguments(node.args, ctx, 1 if is_method else 0)
        args_temp = []
        for idx, arg in enumerate(args):
            arg_tpname = type_transformer(arg.content, ctx, arg.node)
            ctx.symbol[arg.node.arg] = arg_tpname
            args_temp.append(f'{arg_tpname} {arg.node.arg}')

        args_str = ', '.join(args_temp)

        # [XXX] Now we know the types of arguments, and can utilize them for body compilation
        body_str = body_transformer(node.body, ctx, brace_on_ellipsis=False)

        content = f'{returns_str} {func_name}({args_str}){body_str}'

        return transformer.FunctionDefOut(node, content)

    @dataclass
    class ClassDefOut(stmtOut):
        node: ast.ClassDef
        content: str

    @staticmethod
    def from_ClassDef(node: ast.ClassDef, ctx: CompilerContext) -> ClassDefOut:

        body_strs = []
        for stmt in node.body:
            if isinstance(stmt, ast.FunctionDef):
                out = transformer.from_FunctionDef(stmt, ctx, node.name)
                if out.is_shader_func:
                    body_strs.append(out.content)

        body_joiner = '\n\n' if ctx.config['pretty_compile'] else ''
        body_str = body_joiner.join(body_strs)

        content = ''
        if body_str == '...':
            content = ''
        else:
            content = body_str

        return transformer.ClassDefOut(node, content)

    @dataclass
    class ReturnOut(stmtOut):
        node: ast.Return
        content: str

    @staticmethod
    def from_Return(node: ast.Return, ctx: CompilerContext) -> ReturnOut:

        content = 'return;'
        if node.value:
            out = compile_expr(node.value, ctx)
            if out.content != "None":
                content = f'return {out.content};'

        content = get_stmt_indent(node, ctx) + content
        return transformer.ReturnOut(node, content)

    @dataclass
    class AssignOut(stmtOut):
        node: ast.Assign
        content: str

    @staticmethod
    def from_Assign(node: ast.Assign, ctx: CompilerContext) -> AssignOut:
        ''' '''

        ''' Multiple Assignment

        Currently, we forbid the Multiple Assignment in pyshader codes for the following reasons.

        1. Difficulty to distinguish unwanted unpacking from multiple assignments

        At least on the AST, multiple assignments have two forms: normal multiple assignment, and unpacking.
        Normal multiple assignment, for example, is such statements like `a = b = 1`, and unpacking is like
        `a, b = (1, 2)`. Unpacking is clearly not compilable in GLSL, and we'd like to avoid that one. However,
        both assignments are hard to be distinguished on AST. Perhaps, you may think that we can do it depending
        on whether `ast.Assign.value` is tuple/list or not. But, think about an expression like `a, b = func()`.
        In such cases where the `ast.Assign.value` is `ast.Call`, we will immediately lose our basis for judgment.
        We cannot determine the return type of `ast.Call` on the AST. In order to solve these issues, we have to
        tackle on numerous messy stuff like symbol resolution, and so on. That must be a relatively arduous task for us.

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

        content = get_stmt_indent(node, ctx) + f'{targets_str} = {value_str};'

        return transformer.AssignOut(node, content)

    @dataclass
    class AugAssignOut(stmtOut):
        node: ast.AugAssign
        content: str

    @staticmethod
    def from_AugAssign(node: ast.AugAssign, ctx: CompilerContext) -> AugAssignOut:

        target_str = compile_expr(node.target, ctx).content
        op_str = binop_transformer(node.op)
        value_str = compile_expr(node.value, ctx).content

        content = get_stmt_indent(node, ctx) + f'{target_str} {op_str}= {value_str};'

        return transformer.AugAssignOut(node, content)

    @dataclass
    class AnnAssignOut(stmtOut):
        node: ast.AnnAssign
        content: str

    @staticmethod
    def from_AnnAssign(node: ast.AnnAssign, ctx: CompilerContext) -> AnnAssignOut:

        if not node.simple:
            # For the term `simple`, see https://docs.python.org/ja/3/library/ast.html#ast.AnnAssign
            raise CompileError('Non simple Assignment is forbidden')

        target_str = compile_expr(node.target, ctx).content
        ann_str = transformer.from_annotation(node.annotation, ctx)

        content = f'{ann_str} {target_str};'

        if node.value:
            value_str = compile_expr(node.value, ctx).content
            content = f'{ann_str} {target_str} = {value_str};'

        content = get_stmt_indent(node, ctx) + content

        return transformer.AnnAssignOut(node, content)

    @dataclass
    class ForOut(stmtOut):
        node: ast.For
        content: str

    @staticmethod
    def from_For(node: ast.For, ctx: CompilerContext) -> ForOut:

        iter_out = compile_expr(node.iter, ctx)
        if not isinstance(iter_out, transformer.CallOut) or not iter_out.content.startswith('range('):
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

        body_str = body_transformer(node.body, ctx)

        content = get_stmt_indent(node, ctx) + \
            f'for(int {idx}=({idx_start});{idx}<({idx_length});{idx}+=({idx_step})){body_str}'

        return transformer.ForOut(node, content)

    @dataclass
    class WhileOut(stmtOut):
        node: ast.While
        content: str

    @staticmethod
    def from_While(node: ast.While, ctx: CompilerContext) -> WhileOut:

        if len(node.orelse) > 0:
            raise CompileError('Else with while is not supported')

        test_str = compile_expr(node.test, ctx).content
        body_str = body_transformer(node.body, ctx)

        content = get_stmt_indent(node, ctx) + f'while({test_str}){body_str}'

        return transformer.WhileOut(node, content)

    @dataclass
    class IfOut(stmtOut):
        node: ast.If
        content: str

    @staticmethod
    def from_If(node: ast.If, ctx: CompilerContext) -> IfOut:

        test_str = compile_expr(node.test, ctx).content
        body_str = body_transformer(node.body, ctx)

        content = f'if({test_str}){body_str}'

        if len(node.orelse) > 0:
            else_str = body_transformer(node.orelse, ctx)
            content += f'else{else_str}'

        content = get_stmt_indent(node, ctx) + content

        return transformer.IfOut(node, content)

    @dataclass
    class ExprOut(stmtOut):
        node: ast.Expr
        content: str

    @staticmethod
    def from_Expr(node: ast.Expr, ctx: CompilerContext) -> ExprOut:

        out = compile_expr(node.value, ctx)
        content = get_stmt_indent(node, ctx) + out.content + ';'

        return transformer.ExprOut(node, content)

    @dataclass
    class BreakOut(stmtOut):
        node: ast.Break
        content: str

    @staticmethod
    def from_Break(node: ast.Break, ctx: CompilerContext) -> BreakOut:

        content = get_stmt_indent(node, ctx) + 'break;'
        return transformer.BreakOut(node, content)

    @dataclass
    class ContinueOut(stmtOut):
        node: ast.Continue
        content: str

    @staticmethod
    def from_Continue(node: ast.Continue, ctx: CompilerContext) -> ContinueOut:

        content = get_stmt_indent(node, ctx) + 'continue;'
        return transformer.ContinueOut(node, content)

    ''' Expressions '''

    @dataclass
    class exprOut(NodeOut):
        node: ast.expr
        content: str

    @dataclass
    class BoolOpOut(exprOut):
        node: ast.BoolOp
        content: str

    @staticmethod
    def from_BoolOp(node: ast.BoolOp, ctx: CompilerContext) -> BoolOpOut:

        op_str = boolop_transformer(node.op)

        value_strs = []
        for value in node.values:
            value_strs.append(f'({compile_expr(value, ctx).content})')

        content = f' {op_str} '.join(value_strs)
        return transformer.BoolOpOut(node, content)

    @dataclass
    class UnaryOpOut(exprOut):
        node: ast.UnaryOp
        content: str

    @staticmethod
    def from_UnaryOp(node: ast.UnaryOp, ctx: CompilerContext) -> UnaryOpOut:

        op_str = unaryop_transformer(node.op)
        operand_str = compile_expr(node.operand, ctx).content
        content = f'{op_str}{operand_str}'

        return transformer.UnaryOpOut(node, content)

    @dataclass
    class BinOpOut(exprOut):
        node: ast.BinOp
        content: str

    @staticmethod
    def from_BinOp(node: ast.BinOp, ctx: CompilerContext) -> BinOpOut:

        op_str = binop_transformer(node.op)

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

        return transformer.BinOpOut(node, content)

    @dataclass
    class IfExpOut(exprOut):
        node: ast.IfExp
        content: str

    @staticmethod
    def from_IfExp(node: ast.IfExp, ctx: CompilerContext) -> IfExpOut:

        test_str = compile_expr(node.test, ctx).content
        body_str = compile_expr(node.body, ctx).content
        else_str = compile_expr(node.orelse, ctx).content

        content = f'{test_str} ? {body_str} : {else_str}'

        return transformer.IfExpOut(node, content)

    @dataclass
    class CompareOut(exprOut):
        node: ast.Compare
        content: str

    @staticmethod
    def from_Compare(node: ast.Compare, ctx: CompilerContext) -> CompareOut:

        if len(node.ops) > 1 or len(node.comparators) > 1:
            raise CompileError((
                'Multiple comparison operators in one expression is not supported. ' +
                'Consider split the expression like `1 <= a < 10` ==> `1 <= a and a < 10`.'
            ))

        op_str = cmpop_transformer(node.ops[0])

        left_str = compile_expr(node.left, ctx).content
        right_str = compile_expr(node.comparators[0], ctx).content
        content = f'({left_str}) {op_str} ({right_str})'

        return transformer.CompareOut(node, content)

    @dataclass
    class CallOut(exprOut):
        node: ast.Call
        args: list[str]
        content: str

    @staticmethod
    def from_Call(node: ast.Call, ctx: CompilerContext) -> CallOut:

        func_str = compile_expr(node.func, ctx).content
        args = [compile_expr(arg, ctx).content for arg in node.args]
        args_str = ', '.join(args)

        # [TODO] maybe we should explicitly forbid the use of keyword args, *args, **kwargs

        content = f'{func_str}({args_str})'

        return transformer.CallOut(node, args=args, content=content)

    @dataclass
    class ConstantOut(exprOut):
        node: ast.Constant
        content: str

    @staticmethod
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

        return transformer.ConstantOut(node, content)

    @dataclass
    class AttributeOut(exprOut):
        node: ast.Attribute
        content: str

    # [TODO] needs refactoring
    @staticmethod
    def from_Attribute(node: ast.Attribute, ctx: CompilerContext) -> AttributeOut:

        value_str = compile_expr(node.value, ctx).content
        attr_str = str(node.attr)

        if value_str in ctx.symbol:
            value_tpname: str = ctx.symbol[value_str]
            if has_params_qualifier(value_tpname):
                # perhaps this cond always holds
                if attr_str == 'value':
                    attr_str = ''
                    content = f'{value_str}'
                    return transformer.AttributeOut(node, content)
        if value_str == 'gl':
            if hasattr(_gl, attr_str):
                content = f'{attr_str}'
                return transformer.AttributeOut(node, content)
        if value_str == 'self':
            if attr_str in ctx.cls_symbol and ctx.cls_symbol[attr_str][0] == 'instancemethod' and ctx.shmod_name:
                content = mangle_shader_func(ctx.shmod_name, attr_str)
                return transformer.AttributeOut(node, content)
            else:
                content = f'{attr_str}'
                return transformer.AttributeOut(node, content)

        if value_str in ctx.cls_symbol:
            value_tpname: str = ctx.cls_symbol[value_str][0]
            if value_tpname.startswith('uniform'):
                if attr_str == 'value':
                    content = f'{value_str}'
                    return transformer.AttributeOut(node, content)

        # [TODO] change this impl later for symbol resolution based one
        if ctx.shmod_name and value_str == ctx.shmod_name:
            content = mangle_shader_func(value_str, attr_str)
            return transformer.AttributeOut(node, content)

        content = f'{value_str}.{attr_str}'
        return transformer.AttributeOut(node, content)

    @dataclass
    class SubscriptOut(exprOut):
        node: ast.Subscript
        content: str

    @staticmethod
    def from_Subscript(node: ast.Subscript, ctx: CompilerContext) -> SubscriptOut:

        value_str = compile_expr(node.value, ctx).content
        slice_str = compile_expr(node.slice, ctx).content

        content = f'{value_str}[{slice_str}]'

        return transformer.SubscriptOut(node, content)

    @dataclass
    class NameOut(exprOut):
        node: ast.Name
        content: str

    @staticmethod
    def from_Name(node: ast.Name, ctx: CompilerContext) -> NameOut:
        content = ''
        if not hasattr(node, 'id'):
            content = 'None'
        else:
            content = str(node.id)
        return transformer.NameOut(node, content)

    @dataclass
    class ListOut(exprOut):
        node: ast.List
        content: str

    @staticmethod
    def from_List(node: ast.List, ctx: CompilerContext) -> ListOut:

        elts_str = ', '.join([compile_expr(elt, ctx).content for elt in node.elts])
        content = f'[{elts_str}]'

        return transformer.ListOut(node, content)

    @dataclass
    class TupleOut(exprOut):
        node: ast.Tuple
        content: str

    @staticmethod
    def from_Tuple(node: ast.Tuple, ctx: CompilerContext) -> TupleOut:

        elts_str = ', '.join([compile_expr(elt, ctx).content for elt in node.elts])
        content = f'({elts_str})'

        return transformer.TupleOut(node, content)

    @dataclass
    class SliceOut(exprOut):
        node: ast.Slice
        content: str

    @staticmethod
    def from_Slice(node: ast.Slice, ctx: CompilerContext) -> SliceOut:

        lower_str = compile_expr(node.lower, ctx).content if node.lower else ''
        upper_str = compile_expr(node.upper, ctx).content if node.upper else ''
        step_str = compile_expr(node.step, ctx).content if node.step else ''

        content = f'{lower_str}:{upper_str}'
        if step_str != '':
            content += f':{step_str}'

        return transformer.SliceOut(node, content)

    ''' Function Arguments '''

    @staticmethod
    def from_arguments(node: ast.arguments, ctx: CompilerContext, args_offset: int = 0) -> list[argOut]:
        # posonlyargs, kwonlyargs, vararg, kwargs, kw_defaults, defaults are just ignored for now
        return [transformer.from_arg(ag, ctx) for ag in node.args[args_offset:]]

    @dataclass
    class argOut(NodeOut):
        node: ast.arg
        content: str

    @staticmethod
    def from_arg(node: ast.arg, ctx: CompilerContext) -> argOut:
        if not node.annotation:
            raise CompileError('All arguments must be explicitly type-hinted!')
        content = transformer.from_annotation(node.annotation, ctx)
        return transformer.argOut(node, content)

    ''' Others '''

    @staticmethod
    def from_annotation(node: Union[ast.Name, ast.Constant, ast.Attribute, ast.expr], ctx: CompilerContext) -> str:
        tp_name: str = ''
        if isinstance(node, ast.Name):
            tp_name = transformer.from_Name(node, ctx).content
        elif isinstance(node, ast.Constant):
            tp_name = str(transformer.from_Constant(node, ctx))
        elif isinstance(node, ast.Attribute):
            tp_name = transformer.from_Attribute(node, ctx).content
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


def compile_stmt(node: ast.stmt, ctx: CompilerContext) -> transformer.stmtOut:

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
        return transformer.from_FunctionDef(node, ctx)
    if isinstance(node, ast.Return):
        return transformer.from_Return(node, ctx)
    if isinstance(node, ast.Assign):
        return transformer.from_Assign(node, ctx)
    if isinstance(node, ast.AugAssign):
        return transformer.from_AugAssign(node, ctx)
    if isinstance(node, ast.AnnAssign):
        return transformer.from_AnnAssign(node, ctx)
    if isinstance(node, ast.For):
        return transformer.from_For(node, ctx)
    if isinstance(node, ast.While):
        return transformer.from_While(node, ctx)
    if isinstance(node, ast.If):
        return transformer.from_If(node, ctx)
    if isinstance(node, ast.Expr):
        return transformer.from_Expr(node, ctx)
    if isinstance(node, ast.Break):
        return transformer.from_Break(node, ctx)
    if isinstance(node, ast.Continue):
        return transformer.from_Continue(node, ctx)

    if any_instance(node, never_types):
        raise CompileError(f'The statement `{type(node)}` is forbidden')

    raise CompileError(f'Not implemented statement `{type(node)}` found')


def compile_expr(node: ast.expr, ctx: CompilerContext) -> transformer.exprOut:

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
        return transformer.from_BoolOp(node, ctx)
    if isinstance(node, ast.BinOp):
        return transformer.from_BinOp(node, ctx)
    if isinstance(node, ast.UnaryOp):
        return transformer.from_UnaryOp(node, ctx)
    if isinstance(node, ast.IfExp):
        return transformer.from_IfExp(node, ctx)
    if isinstance(node, ast.Compare):
        return transformer.from_Compare(node, ctx)
    if isinstance(node, ast.Call):
        return transformer.from_Call(node, ctx)
    if isinstance(node, ast.Constant):
        return transformer.from_Constant(node, ctx)
    if isinstance(node, ast.Attribute):
        return transformer.from_Attribute(node, ctx)
    if isinstance(node, ast.Subscript):
        return transformer.from_Subscript(node, ctx)
    if isinstance(node, ast.Name):
        return transformer.from_Name(node, ctx)
    if isinstance(node, ast.List):
        return transformer.from_List(node, ctx)
    if isinstance(node, ast.Tuple):
        return transformer.from_Tuple(node, ctx)
    if isinstance(node, ast.Slice):
        return transformer.from_Slice(node, ctx)

    if any_instance(node, (ast.FormattedValue, ast.JoinedStr)):
        raise CompileError('f-string is not supported as for now')

    if any_instance(node, never_types):
        raise CompileError(f'The expression `{type(node)}` is forbidden')

    raise CompileError(f'Not implemented expression `{type(node)}` found')


def visit_all(node: ast.AST):
    class Visitor(ast.NodeVisitor):
        def visit(self, node):
            print(node)
            super().visit(node)

    Visitor().visit(node)


def dump_ast(node: ast.AST) -> str:
    return ast.dump(node, indent=2)


def unwrap_shader_func(fn: Callable) -> Optional[ast.FunctionDef]:

    if not(hasattr(fn, '__closure__') and fn.__closure__ and len(fn.__closure__) > 0):
        return None

    deco_fn = fn.__closure__[0].cell_contents

    py_src = __priv.get_source(deco_fn)
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


def unwrap_static_shader_func(fn: Callable) -> ast.FunctionDef:
    py_src = __priv.get_source(fn)
    root = ast.parse(py_src)
    return get_function_def(root)


def compile_shader_func(
        fn: Callable,
        config: CompilerConfig.Config = CompilerConfig.default()) -> _TGLSL:

    ctx = CompilerContext(config)

    func_def = unwrap_shader_func(fn)
    if not func_def:
        raise CompileError('Shader function/method must be decorated properly')

    out = transformer.from_FunctionDef(func_def, ctx)
    return out.content


def __compile_shader_func2(func_def: ast.FunctionDef, ctx: CompilerContext) -> _TGLSL:
    out = transformer.from_FunctionDef(func_def, ctx)
    return out.content


def class_symbol_analysis(sh_mod: ShaderModule, ctx: CompilerContext) -> None:

    # collect annotated instance variables
    for fld_name, fld_type in sh_mod.__class__.__annotations__.items():
        if isinstance(fld_type, tp._GenericAlias):  # type: ignore
            basic_tpname: str = str(fld_type.__origin__.__name__)  # type:ignore
            ctx.cls_symbol[fld_name] = (basic_tpname, fld_type)
        else:
            ctx.cls_symbol[fld_name] = (str(type(fld_type).__name__), fld_type)

    # collect instance method
    for method_name, method in inspect.getmembers(sh_mod, inspect.ismethod):
        ctx.cls_symbol[method_name] = ('instancemethod', method)


def compile_shader_module(
        sh_mod: ShaderModule,
        config: CompilerConfig.Config = CompilerConfig.default()) -> _TGLSL:
    klass = sh_mod.__class__

    py_src = __priv.get_source(klass)
    root = ast.parse(py_src)
    class_def = get_class_def(root)

    ctx = CompilerContext(config)

    ctx.shmod_name = klass.__name__
    class_symbol_analysis(sh_mod, ctx)

    out = transformer.from_ClassDef(class_def, ctx)

    return out.content
