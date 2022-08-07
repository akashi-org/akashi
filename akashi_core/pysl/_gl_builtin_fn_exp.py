# pyright: reportPrivateUsage=false
from __future__ import annotations

from typing import overload, Any, TYPE_CHECKING

if TYPE_CHECKING:
    from akashi_core.pysl._gl_vec import _TGenType, _TGenDType

''' 
    Exponential Functions (GLSL 4.20.11 specification, section 8.2)
'''


def pow(x: _TGenType, y: _TGenType) -> _TGenType: ...


def exp(x: _TGenType) -> _TGenType: ...


def log(x: _TGenType) -> _TGenType: ...


def exp2(x: _TGenType) -> _TGenType: ...


def log2(x: _TGenType) -> _TGenType: ...


@overload
def sqrt(x: _TGenDType) -> _TGenDType: ...


@overload
def sqrt(x: _TGenType) -> _TGenType: ...


def sqrt(x: Any) -> Any: ...


@overload
def inversesqrt(x: _TGenDType) -> _TGenDType: ...


@overload
def inversesqrt(x: _TGenType) -> _TGenType: ...


def inversesqrt(x: Any) -> Any: ...
