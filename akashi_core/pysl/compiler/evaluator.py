from __future__ import annotations

from .items import CompilerContext

import typing as tp
import ast


def eval_node(node: ast.expr, ctx: CompilerContext) -> str:

    eval_body = ast.unparse(node)
    eval_res = eval(eval_body, ctx.global_symbol, ctx.eval_local_symbol)
    if isinstance(eval_res, bool):
        return str(eval_res).lower()
    return str(eval_res)
