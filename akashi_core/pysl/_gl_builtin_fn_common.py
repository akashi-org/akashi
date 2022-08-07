# pyright: reportPrivateUsage=false
from __future__ import annotations

from typing import overload, Any, TYPE_CHECKING, TypeVar, Annotated

if TYPE_CHECKING:
    from akashi_core.pysl._gl_vec import _TGenType, _TGenDType, _TGenIType, _TGenBType, _TGenUType
    from akashi_core.pysl._gl_common import uint, double


_T = TypeVar('_T')
_out_p = Annotated[_T, 'out_p']  # NB: Do not use the name `out_p`
_in_p = Annotated[_T, 'in_p']  # NB: Do not use the name `in_p`

''' 
    8.3 Common Functions
'''


@overload
def abs(x: _TGenDType) -> _TGenDType: ...


@overload
def abs(x: _TGenIType) -> _TGenIType: ...


@overload
def abs(x: _TGenType) -> _TGenType: ...


def abs(x) -> Any: ...


@overload
def sign(x: _TGenDType) -> _TGenDType: ...


@overload
def sign(x: _TGenIType) -> _TGenIType: ...


@overload
def sign(x: _TGenType) -> _TGenType: ...


def sign(x) -> Any: ...


@overload
def floor(x: _TGenDType) -> _TGenDType: ...


@overload
def floor(x: _TGenType) -> _TGenType: ...


def floor(x: Any) -> Any: ...


@overload
def trunc(x: _TGenDType) -> _TGenDType: ...


@overload
def trunc(x: _TGenType) -> _TGenType: ...


def trunc(x: Any) -> Any: ...


@overload
def round(x: _TGenDType) -> _TGenDType: ...


@overload
def round(x: _TGenType) -> _TGenType: ...


def round(x: Any) -> Any: ...


@overload
def roundEven(x: _TGenDType) -> _TGenDType: ...


@overload
def roundEven(x: _TGenType) -> _TGenType: ...


def roundEven(x: Any) -> Any: ...


@overload
def ceil(x: _TGenDType) -> _TGenDType: ...


@overload
def ceil(x: _TGenType) -> _TGenType: ...


def ceil(x: Any) -> Any: ...


@overload
def fract(x: _TGenDType) -> _TGenDType: ...


@overload
def fract(x: _TGenType) -> _TGenType: ...


def fract(x: Any) -> Any: ...


@overload
def mod(x: _TGenDType, y: double) -> _TGenDType: ...


@overload
def mod(x: _TGenDType, y: _TGenDType) -> _TGenDType: ...


@overload
def mod(x: _TGenType, y: float) -> _TGenType: ...


@overload
def mod(x: _TGenType, y: _TGenType) -> _TGenType: ...


def mod(x: Any, y: Any) -> Any: ...


@overload
def modf(x: _TGenDType, i: _out_p[_TGenDType]) -> _TGenDType: ...


@overload
def modf(x: _TGenType, i: _out_p[_TGenType]) -> _TGenType: ...


def modf(x: Any, i: Any) -> Any: ...


@overload
def min(x: _TGenUType, y: _TGenUType) -> _TGenUType: ...


@overload
def min(x: _TGenUType, y: uint) -> _TGenUType: ...


@overload
def min(x: _TGenDType, y: _TGenDType) -> _TGenDType: ...


@overload
def min(x: _TGenDType, y: double) -> _TGenDType: ...


@overload
def min(x: _TGenIType, y: _TGenIType) -> _TGenIType: ...


@overload
def min(x: _TGenIType, y: int) -> _TGenIType: ...


@overload
def min(x: _TGenType, y: _TGenType) -> _TGenType: ...


@overload
def min(x: _TGenType, y: float) -> _TGenType: ...


def min(x: Any, y: Any) -> Any: ...


@overload
def max(x: _TGenUType, y: _TGenUType) -> _TGenUType: ...


@overload
def max(x: _TGenUType, y: uint) -> _TGenUType: ...


@overload
def max(x: _TGenDType, y: _TGenDType) -> _TGenDType: ...


@overload
def max(x: _TGenDType, y: double) -> _TGenDType: ...


@overload
def max(x: _TGenIType, y: _TGenIType) -> _TGenIType: ...


@overload
def max(x: _TGenIType, y: int) -> _TGenIType: ...


@overload
def max(x: _TGenType, y: _TGenType) -> _TGenType: ...


@overload
def max(x: _TGenType, y: float) -> _TGenType: ...


def max(x: Any, y: Any) -> Any: ...


@overload
def clamp(x: _TGenUType, minVal: _TGenUType, maxVal: _TGenUType) -> _TGenUType: ...


@overload
def clamp(x: _TGenUType, minVal: uint, maxVal: uint) -> _TGenUType: ...


@overload
def clamp(x: _TGenDType, minVal: _TGenDType, maxVal: _TGenDType) -> _TGenDType: ...


@overload
def clamp(x: _TGenDType, minVal: double, maxVal: double) -> _TGenDType: ...


@overload
def clamp(x: _TGenIType, minVal: _TGenIType, maxVal: _TGenIType) -> _TGenIType: ...


@overload
def clamp(x: _TGenIType, minVal: int, maxVal: int) -> _TGenIType: ...


@overload
def clamp(x: _TGenType, minVal: _TGenType, maxVal: _TGenType) -> _TGenType: ...


@overload
def clamp(x: _TGenType, minVal: float, maxVal: float) -> _TGenType: ...


def clamp(x: Any, minVal: Any, maxVal: Any) -> Any: ...


@overload
def mix(x: _TGenDType, y: _TGenDType, a: _TGenBType) -> _TGenDType: ...


@overload
def mix(x: _TGenDType, y: _TGenDType, a: _TGenDType) -> _TGenDType: ...


@overload
def mix(x: _TGenDType, y: _TGenDType, a: double) -> _TGenDType: ...


@overload
def mix(x: _TGenType, y: _TGenType, a: _TGenBType) -> _TGenType: ...


@overload
def mix(x: _TGenType, y: _TGenType, a: _TGenType) -> _TGenType: ...


@overload
def mix(x: _TGenType, y: _TGenType, a: float) -> _TGenType: ...


def mix(x: Any, y: Any, a: Any) -> Any: ...


@overload
def step(edge: _TGenDType, x: _TGenDType) -> _TGenDType: ...


@overload
def step(edge: double, x: _TGenDType) -> _TGenDType: ...


@overload
def step(edge: _TGenType, x: _TGenType) -> _TGenType: ...


@overload
def step(edge: float, x: _TGenType) -> _TGenType: ...


def step(edge: Any, x: Any) -> Any: ...


@overload
def smoothstep(edge0: _TGenDType, edge1: _TGenDType, x: _TGenDType) -> _TGenDType: ...


@overload
def smoothstep(edge0: double, edge1: double, x: _TGenDType) -> _TGenDType: ...


@overload
def smoothstep(edge0: _TGenType, edge1: _TGenType, x: _TGenType) -> _TGenType: ...


@overload
def smoothstep(edge0: float, edge1: float, x: _TGenType) -> _TGenType: ...


def smoothstep(edge0: Any, edge1: Any, x: Any) -> Any: ...


@overload
def isnan(x: _TGenDType) -> _TGenBType: ...


@overload
def isnan(x: _TGenType) -> _TGenBType: ...


def isnan(x: Any) -> _TGenBType: ...


@overload
def isinf(x: _TGenDType) -> _TGenBType: ...


@overload
def isinf(x: _TGenType) -> _TGenBType: ...


def isinf(x: Any) -> _TGenBType: ...


def floatBitsToInt(value: _TGenType) -> _TGenIType: ...


def floatBitsToUint(value: _TGenType) -> _TGenUType: ...


def intBitsToFloat(value: _TGenIType) -> _TGenType: ...


def uintBitsToFloat(value: _TGenUType) -> _TGenType: ...


@overload
def fma(a: _TGenDType, b: _TGenDType, c: _TGenDType) -> _TGenDType: ...


@overload
def fma(a: _TGenType, b: _TGenType, c: _TGenType) -> _TGenType: ...


def fma(a: Any, b: Any, c: Any) -> Any: ...


@overload
def frexp(x: _TGenDType, exp: _out_p[_TGenIType]) -> _TGenDType: ...


@overload
def frexp(x: _TGenType, exp: _out_p[_TGenIType]) -> _TGenType: ...


def frexp(x: Any, exp: Any) -> Any: ...


@overload
def ldexp(x: _TGenDType, exp: _in_p[_TGenIType]) -> _TGenDType: ...


@overload
def ldexp(x: _TGenType, exp: _in_p[_TGenIType]) -> _TGenType: ...


def ldexp(x: Any, exp: Any) -> Any: ...
