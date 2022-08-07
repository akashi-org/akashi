# pyright: reportPrivateUsage=false
from __future__ import annotations

from typing import overload, Any, TYPE_CHECKING, TypeVar

if TYPE_CHECKING:
    from akashi_core.pysl._gl_common import double

    from akashi_core.pysl._gl_mat import (
        mat2, mat2x3, mat2x4,
        mat3x2, mat3, mat3x4,
        mat4x2, mat4x3, mat4,
        dmat2, dmat2x3, dmat2x4,
        dmat3x2, dmat3, dmat3x4,
        dmat4x2, dmat4x3, dmat4,
    )
    from akashi_core.pysl._gl_vec import (
        dvec2, dvec3, dvec4,
        vec2, vec3, vec4,
    )


_TGenMat = TypeVar('_TGenMat',
                   "mat2", "mat2x3", "mat2x4",
                   "mat3x2", "mat3", "mat3x4",
                   "mat4x2", "mat4x3", "mat4",
                   "dmat2", "dmat2x3", "dmat2x4",
                   "dmat3x2", "dmat3", "dmat3x4",
                   "dmat4x2", "dmat4x3", "dmat4")


''' 
    8.6 Matrix Functions
'''


def matrixCompMult(x: _TGenMat, y: _TGenMat) -> _TGenMat: ...

# --- float


@overload
def outerProduct(c: 'vec2', r: 'vec2') -> 'mat2': ...
@overload
def outerProduct(c: 'vec3', r: 'vec3') -> 'mat3': ...
@overload
def outerProduct(c: 'vec4', r: 'vec4') -> 'mat4': ...
@overload
def outerProduct(c: 'vec3', r: 'vec2') -> 'mat2x3': ...
@overload
def outerProduct(c: 'vec2', r: 'vec3') -> 'mat3x2': ...
@overload
def outerProduct(c: 'vec4', r: 'vec2') -> 'mat2x4': ...
@overload
def outerProduct(c: 'vec2', r: 'vec4') -> 'mat4x2': ...
@overload
def outerProduct(c: 'vec4', r: 'vec3') -> 'mat3x4': ...
@overload
def outerProduct(c: 'vec3', r: 'vec4') -> 'mat4x3': ...

# -- double ---


@overload
def outerProduct(c: 'dvec2', r: 'dvec2') -> 'dmat2': ...
@overload
def outerProduct(c: 'dvec3', r: 'dvec3') -> 'dmat3': ...
@overload
def outerProduct(c: 'dvec4', r: 'dvec4') -> 'dmat4': ...
@overload
def outerProduct(c: 'dvec3', r: 'dvec2') -> 'dmat2x3': ...
@overload
def outerProduct(c: 'dvec2', r: 'dvec3') -> 'dmat3x2': ...
@overload
def outerProduct(c: 'dvec4', r: 'dvec2') -> 'dmat2x4': ...
@overload
def outerProduct(c: 'dvec2', r: 'dvec4') -> 'dmat4x2': ...
@overload
def outerProduct(c: 'dvec4', r: 'dvec3') -> 'dmat3x4': ...
@overload
def outerProduct(c: 'dvec3', r: 'dvec4') -> 'dmat4x3': ...


def outerProduct(c: Any, r: Any) -> Any: ...


# --- float

@overload
def transpose(m: 'mat2') -> 'mat2': ...
@overload
def transpose(m: 'mat3') -> 'mat3': ...
@overload
def transpose(m: 'mat4') -> 'mat4': ...
@overload
def transpose(m: 'mat3x2') -> 'mat2x3': ...
@overload
def transpose(m: 'mat2x3') -> 'mat3x2': ...
@overload
def transpose(m: 'mat2x4') -> 'mat4x2': ...
@overload
def transpose(m: 'mat4x2') -> 'mat2x4': ...
@overload
def transpose(m: 'mat4x3') -> 'mat3x4': ...
@overload
def transpose(m: 'mat3x4') -> 'mat4x3': ...


# --- double

@overload
def transpose(m: 'dmat2') -> 'dmat2': ...
@overload
def transpose(m: 'dmat3') -> 'dmat3': ...
@overload
def transpose(m: 'dmat4') -> 'dmat4': ...
@overload
def transpose(m: 'dmat3x2') -> 'dmat2x3': ...
@overload
def transpose(m: 'dmat2x3') -> 'dmat3x2': ...
@overload
def transpose(m: 'dmat2x4') -> 'dmat4x2': ...
@overload
def transpose(m: 'dmat4x2') -> 'dmat2x4': ...
@overload
def transpose(m: 'dmat4x3') -> 'dmat3x4': ...
@overload
def transpose(m: 'dmat3x4') -> 'dmat4x3': ...


def transpose(m: Any) -> Any: ...

# --- float


@overload
def determinant(m: 'mat2') -> float: ...
@overload
def determinant(m: 'mat3') -> float: ...
@overload
def determinant(m: 'mat4') -> float: ...

# --- double


@overload
def determinant(m: 'dmat2') -> 'double': ...
@overload
def determinant(m: 'dmat3') -> 'double': ...
@overload
def determinant(m: 'dmat4') -> 'double': ...


def determinant(m: Any) -> Any: ...


# --- float


@overload
def inverse(m: 'mat2') -> 'mat2': ...
@overload
def inverse(m: 'mat3') -> 'mat3': ...
@overload
def inverse(m: 'mat4') -> 'mat4': ...

# --- double


@overload
def inverse(m: 'dmat2') -> 'dmat2': ...
@overload
def inverse(m: 'dmat3') -> 'dmat3': ...
@overload
def inverse(m: 'dmat4') -> 'dmat4': ...


def inverse(m: Any) -> Any: ...
