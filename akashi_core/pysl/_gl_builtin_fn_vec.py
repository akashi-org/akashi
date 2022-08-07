# pyright: reportPrivateUsage=false
from __future__ import annotations

from typing import overload, Any, TYPE_CHECKING, TypeVar

if TYPE_CHECKING:
    from akashi_core.pysl._gl_vec import (
        bvec2, bvec3, bvec4,
        ivec2, ivec3, ivec4,
        uvec2, uvec3, uvec4,
        dvec2, dvec3, dvec4,
        vec2, vec3, vec4,
    )

''' 
    8.7 Vector Relational Functions
'''

_TGenBVec = TypeVar('_TGenBVec', 'bvec2', 'bvec3', 'bvec4')
_TGenUVec = TypeVar('_TGenUVec', 'uvec2', 'uvec3', 'uvec4')
_TGenIVec = TypeVar('_TGenIVec', 'ivec2', 'ivec3', 'ivec4')
_TGenVec = TypeVar('_TGenVec', 'vec2', 'vec3', 'vec4', 'dvec2', 'dvec3', 'dvec4')


@overload
def lessThan(x: _TGenUVec, y: _TGenUVec) -> _TGenBVec: ...


@overload
def lessThan(x: _TGenIVec, y: _TGenIVec) -> _TGenBVec: ...


@overload
def lessThan(x: _TGenVec, y: _TGenVec) -> _TGenBVec: ...


def lessThan(x: Any, y: Any) -> _TGenBVec: ...


@overload
def lessThanEqual(x: _TGenUVec, y: _TGenUVec) -> _TGenBVec: ...


@overload
def lessThanEqual(x: _TGenIVec, y: _TGenIVec) -> _TGenBVec: ...


@overload
def lessThanEqual(x: _TGenVec, y: _TGenVec) -> _TGenBVec: ...


def lessThanEqual(x: Any, y: Any) -> _TGenBVec: ...


@overload
def greaterThan(x: _TGenUVec, y: _TGenUVec) -> _TGenBVec: ...


@overload
def greaterThan(x: _TGenIVec, y: _TGenIVec) -> _TGenBVec: ...


@overload
def greaterThan(x: _TGenVec, y: _TGenVec) -> _TGenBVec: ...


def greaterThan(x: Any, y: Any) -> _TGenBVec: ...


@overload
def greaterThanEqual(x: _TGenUVec, y: _TGenUVec) -> _TGenBVec: ...


@overload
def greaterThanEqual(x: _TGenIVec, y: _TGenIVec) -> _TGenBVec: ...


@overload
def greaterThanEqual(x: _TGenVec, y: _TGenVec) -> _TGenBVec: ...


def greaterThanEqual(x: Any, y: Any) -> _TGenBVec: ...


@overload
def equal(x: _TGenBVec, y: _TGenBVec) -> _TGenBVec: ...


@overload
def equal(x: _TGenUVec, y: _TGenUVec) -> _TGenBVec: ...


@overload
def equal(x: _TGenIVec, y: _TGenIVec) -> _TGenBVec: ...


@overload
def equal(x: _TGenVec, y: _TGenVec) -> _TGenBVec: ...


def equal(x: Any, y: Any) -> _TGenBVec: ...


@overload
def notEqual(x: _TGenBVec, y: _TGenBVec) -> _TGenBVec: ...


@overload
def notEqual(x: _TGenUVec, y: _TGenUVec) -> _TGenBVec: ...


@overload
def notEqual(x: _TGenIVec, y: _TGenIVec) -> _TGenBVec: ...


@overload
def notEqual(x: _TGenVec, y: _TGenVec) -> _TGenBVec: ...


def notEqual(x: Any, y: Any) -> _TGenBVec: ...


def any_(x: _TGenBVec) -> bool: ...


def all_(x: _TGenBVec) -> bool: ...


def not_(x: _TGenBVec) -> _TGenBVec: ...
