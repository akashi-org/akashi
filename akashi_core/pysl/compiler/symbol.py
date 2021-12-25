# pyright: reportPrivateUsage=false
from __future__ import annotations

from .items import CompilerContext

import typing as tp
import inspect
import sys

if tp.TYPE_CHECKING:
    from akashi_core.pysl.shader import ShaderModule


def buffer_symbol_analysis(sh_mod: 'ShaderModule', ctx: CompilerContext) -> None:

    for mem_name, mem in inspect.getmembers(sh_mod):
        if not(callable(mem)):
            ctx.local_symbol[mem_name] = str(type(mem).__name__)
