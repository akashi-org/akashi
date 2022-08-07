# pyright: reportPrivateUsage=false
from __future__ import annotations

from typing import overload, Any, TYPE_CHECKING

if TYPE_CHECKING:
    from akashi_core.pysl._gl_vec import _TGenType, _TGenDType
    from akashi_core.pysl._gl_common import double
    from akashi_core.pysl._gl_vec import vec3, dvec3

''' 
    8.5 Geometric Functions
'''


@overload
def length(x: _TGenDType) -> double: ...


@overload
def length(x: _TGenType) -> float: ...


def length(x: Any) -> Any: ...


@overload
def distance(p0: _TGenDType, p1: _TGenDType) -> double: ...


@overload
def distance(p0: _TGenType, p1: _TGenType) -> float: ...


def distance(p0: Any, p1: Any) -> Any: ...


@overload
def dot(x: _TGenDType, y: _TGenDType) -> double: ...


@overload
def dot(x: _TGenType, y: _TGenType) -> float: ...


def dot(x: Any, y: Any) -> Any: ...


@overload
def cross(x: 'dvec3', y: 'dvec3') -> 'dvec3': ...


@overload
def cross(x: 'vec3', y: 'vec3') -> 'vec3': ...


def cross(x: Any, y: Any) -> Any: ...


@overload
def normalize(x: _TGenDType) -> _TGenDType: ...


@overload
def normalize(x: _TGenType) -> _TGenType: ...


def normalize(x: Any) -> Any: ...


@overload
def faceforward(N: _TGenDType, I: _TGenDType, Nref: _TGenDType) -> _TGenDType: ...  # noqa: E741


@overload
def faceforward(N: _TGenType, I: _TGenType, Nref: _TGenType) -> _TGenType: ...  # noqa: E741


def faceforward(N: Any, I: Any, Nref: Any) -> Any: ...  # noqa: E741


@overload
def reflect(I: _TGenDType, N: _TGenDType) -> _TGenDType: ...  # noqa: E741


@overload
def reflect(I: _TGenType, N: _TGenType) -> _TGenType: ...  # noqa: E741


def reflect(I: Any, N: Any) -> Any: ...  # noqa: E741


# NB: eta is float!
@overload
def refract(I: _TGenDType, N: _TGenDType, eta: float) -> _TGenDType: ...  # noqa: E741


@overload
def refract(I: _TGenType, N: _TGenType, eta: float) -> _TGenType: ...  # noqa: E741


def refract(I: Any, N: Any, eta: float) -> Any: ...  # noqa: E741
