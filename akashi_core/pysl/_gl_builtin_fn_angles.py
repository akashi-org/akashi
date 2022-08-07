# pyright: reportPrivateUsage=false
from __future__ import annotations

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from akashi_core.pysl._gl_vec import _TGenType

''' 
    8.1 Angle and Trigonometry Functions
'''


def radians(degrees: _TGenType) -> _TGenType: ...


def degrees(radians: _TGenType) -> _TGenType: ...


def sin(angle: _TGenType) -> _TGenType: ...


def cos(angle: _TGenType) -> _TGenType: ...


def tan(angle: _TGenType) -> _TGenType: ...


def asin(x: _TGenType) -> _TGenType: ...


def acos(x: _TGenType) -> _TGenType: ...


def atan(a: _TGenType, b: _TGenType | None = None) -> _TGenType: ...


def sinh(angle: _TGenType) -> _TGenType: ...


def cosh(angle: _TGenType) -> _TGenType: ...


def tanh(angle: _TGenType) -> _TGenType: ...


def asinh(angle: _TGenType) -> _TGenType: ...


def acosh(angle: _TGenType) -> _TGenType: ...


def atanh(angle: _TGenType) -> _TGenType: ...
