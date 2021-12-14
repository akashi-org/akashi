# pyright: reportPrivateUsage=false
from __future__ import annotations

from .items import CompilerContext

import typing as tp
import inspect
import ast
import sys

if tp.TYPE_CHECKING:
    from akashi_core.pysl.shader import ShaderModule


def instance_symbol_analysis(sh_mod: 'ShaderModule', ctx: CompilerContext) -> None:

    # collect annotated instance variables
    for fld_name, fld_type in sh_mod.__class__.__annotations__.items():
        if isinstance(fld_type, tp._GenericAlias):  # type: ignore
            basic_tpname: str = str(fld_type.__origin__.__name__)  # type:ignore
            ctx.cls_symbol[fld_name] = (basic_tpname, fld_type)
        else:
            ctx.cls_symbol[fld_name] = (str(type(fld_type).__name__), fld_type)

    # collect instance method and inherited variables
    for mem_name, mem in inspect.getmembers(sh_mod):
        if inspect.ismethod(mem) and not mem_name.startswith('_'):
            ctx.cls_symbol[mem_name] = ('instancemethod', mem)

        if not(inspect.isfunction(mem)) and not(inspect.ismethod(mem)):
            ctx.cls_symbol[mem_name] = (str(type(mem).__name__), mem)


def class_symbol_analysis(shmod_klass: tp.Type['ShaderModule'], ctx: CompilerContext) -> None:

    # collect annotated variables
    for fld_name, fld_type in shmod_klass.__annotations__.items():
        if isinstance(fld_type, tp._GenericAlias):  # type: ignore
            basic_tpname: str = str(fld_type.__origin__.__name__)  # type:ignore
            ctx.cls_symbol[fld_name] = (basic_tpname, fld_type)
        else:
            ctx.cls_symbol[fld_name] = (str(type(fld_type).__name__), fld_type)

    # collect static method
    for method_name, method in inspect.getmembers(shmod_klass, inspect.ismethod):
        ctx.cls_symbol[method_name] = ('instancemethod', method)


def global_symbol_analysis(shmod_klass: tp.Type['ShaderModule'], ctx: CompilerContext) -> None:

    ctx.global_symbol = vars(sys.modules[shmod_klass.__module__])
