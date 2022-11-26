# pyright: reportPrivateUsage=false
from __future__ import annotations

__all__ = [
    "eval",
    "ceval",
    "inline_stmt",
    "gl_FragCoord",
    "uint", "double",
    "vec2", "vec3", "vec4",
    "dvec2", "dvec3", "dvec4",
    "ivec2", "ivec3", "ivec4",
    "uvec2", "uvec3", "uvec4",
    "bvec2", "bvec3", "bvec4",
    "mat2x2", "dmat2x2", "mat2", "dmat2", "mat2x3", "dmat2x3", "mat2x4", "dmat2x4",
    "mat3x2", "dmat3x2", "mat3x3", "dmat3x3", "mat3", "dmat3", "mat3x4", "dmat3x4",
    "mat4x2", "dmat4x2", "mat4x3", "dmat4x3", "mat4x4", "dmat4x4", "mat4", "dmat4",
    "sampler1D", "isampler1D", "usampler1D", "sampler2D", "isampler2D", "usampler2D", "sampler3D", "isampler3D", "usampler3D",
    "samplerCube", "isamplerCube", "usamplerCube", "sampler2DRect", "isampler2DRect", "usampler2DRect",
    "sampler1DArray", "isampler1DArray", "usampler1DArray", "sampler2DArray", "isampler2DArray", "usampler2DArray",
    "samplerCubeArray", "isamplerCubeArray", "usamplerCubeArray", "samplerBuffer", "isamplerBuffer", "usamplerBuffer",
    "sampler2DMS", "isampler2DMS", "usampler2DMS", "sampler2DMSArray", "isampler2DMSArray", "usampler2DMSArray",
    "samplerCubeArrayShadow", "isamplerCubeArrayShadow", "usamplerCubeArrayShadow",

    "radians", "degrees", "sin", "cos", "tan", "asin", "acos", "atan", "sinh", "cosh", "tanh", "asinh", "acosh", "atanh",
    "pow", "exp", "log", "exp2", "log2", "sqrt", "inversesqrt",
    "abs", "sign", "floor", "trunc", "round", "roundEven", "ceil", "fract", "mod", "modf", "min", "max", "clamp",
    "mix", "step", "smoothstep", "isnan", "isinf", "floatBitsToInt", "floatBitsToUint", "intBitsToFloat", "uintBitsToFloat",
    "fma", "frexp", "ldexp",
    "length", "distance", "dot", "cross", "normalize", "faceforward", "reflect", "refract",
    "matrixCompMult", "outerProduct", "transpose", "determinant", "inverse",
    "lessThan", "lessThanEqual", "greaterThan", "greaterThanEqual", "equal", "notEqual", "any_", "all_", "not_",
    "texture",
    "dFdx", "dFdy", "fwidth",
    "EmitStreamVertex", "EndStreamPrimitive", "EmitVertex", "EndPrimitive"
]


from dataclasses import dataclass
from typing import (
    TypeVar,
    Generic,
    overload,
    Any,
    Optional,
    cast,
    ParamSpec,
    Literal,
    Callable,
    Annotated,
    Type,
    Final,
    NoReturn,
    TYPE_CHECKING
)

from ._gl_common import uint, double

from ._gl_vec import (
    bvec2, bvec3, bvec4,
    ivec2, ivec3, ivec4,
    uvec2, uvec3, uvec4,
    dvec2, dvec3, dvec4,
    vec2, vec3, vec4
)

from ._gl_mat import (
    mat2x2, dmat2x2, mat2, dmat2, mat2x3, dmat2x3, mat2x4, dmat2x4,
    mat3x2, dmat3x2, mat3x3, dmat3x3, mat3, dmat3, mat3x4, dmat3x4,
    mat4x2, dmat4x2, mat4x3, dmat4x3, mat4x4, dmat4x4, mat4, dmat4
)

from ._gl_sampler import (
    sampler1D, isampler1D, usampler1D, sampler2D, isampler2D, usampler2D, sampler3D, isampler3D, usampler3D,
    samplerCube, isamplerCube, usamplerCube, sampler2DRect, isampler2DRect, usampler2DRect,
    sampler1DArray, isampler1DArray, usampler1DArray, sampler2DArray, isampler2DArray, usampler2DArray,
    samplerCubeArray, isamplerCubeArray, usamplerCubeArray, samplerBuffer, isamplerBuffer, usamplerBuffer,
    sampler2DMS, isampler2DMS, usampler2DMS, sampler2DMSArray, isampler2DMSArray, usampler2DMSArray,
    samplerCubeArrayShadow, isamplerCubeArrayShadow, usamplerCubeArrayShadow
)

from ._gl_builtin_fn_angles import (
    radians, degrees, sin, cos, tan, asin, acos, atan, sinh, cosh, tanh, asinh, acosh, atanh
)
from ._gl_builtin_fn_exp import pow, exp, log, exp2, log2, sqrt, inversesqrt
from ._gl_builtin_fn_common import (
    abs, sign, floor, trunc, round, roundEven, ceil, fract, mod, modf, min, max, clamp, mix, step, smoothstep, isnan, isinf,
    floatBitsToInt, floatBitsToUint, intBitsToFloat, uintBitsToFloat, fma, frexp, ldexp
)
# from ._gl_builtin_fn_pack import *
from ._gl_builtin_fn_geometric import length, distance, dot, cross, normalize, faceforward, reflect, refract
from ._gl_builtin_fn_matrix import matrixCompMult, outerProduct, transpose, determinant, inverse
from ._gl_builtin_fn_vec import (
    lessThan, lessThanEqual, greaterThan, greaterThanEqual, equal, notEqual,
    any_, all_, not_
)
# from ._gl_builtin_fn_int import *
from ._gl_builtin_fn_tex import texture
# from ._gl_builtin_fn_atomic import *
# from ._gl_builtin_fn_image import *
from ._gl_builtin_fn_frag import dFdx, dFdy, fwidth
from ._gl_builtin_fn_geom import EmitStreamVertex, EndStreamPrimitive, EmitVertex, EndPrimitive
# from ._gl_builtin_fn_sync import *

from ._gl_builtin_vars import gl_FragCoord

if TYPE_CHECKING:
    from .shader import _NamedEntryFragFn, _NamedEntryPolyFn, _TEntryFnOpaque


_T = TypeVar('_T')


''' Function Parameter qualifiers '''


in_p = Annotated[_T, 'in_p']

out_p = Annotated[_T, 'out_p']

inout_p = Annotated[_T, 'inout_p']


def _default() -> Any:
    return 1.0


uniform = Annotated[_T, 'uniform']


def _uniform_default() -> uniform[Any]:
    return 1.0


in_t = Annotated[_T, 'in_t']


def _in_t_default() -> in_t[Any]:
    return 1.0


out_t = Annotated[_T, 'out_t']


def _out_t_default() -> out_t[Any]:
    return 1.0


''' PYSL extentions '''


def eval(__expr: _T) -> _T:
    ...


def ceval(__expr: _T) -> _T:
    ...


def inline_stmt(glsl_src: str):
    ...


''' Misc '''

frag_color: Type[inout_p[vec4]] = inout_p[vec4]

poly_pos: Type[inout_p[vec4]] = inout_p[vec4]


@dataclass
class _expr(Generic[_T]):

    v: _T

    def tp(self, _tp: Type[_T]) -> '_expr[_T]':
        return self

    # [XXX] concats multiple exprs
    def __or__(self, other: '_expr') -> '_expr':
        return other

    # [XXX] substitutes for assignment(=)
    def __lshift__(self, other: '_expr[_T]') -> '_expr[_T]':
        return self


@dataclass
class _LayerUniform:

    time: Final['uniform'[float]] = _uniform_default()
    global_time: Final['uniform'[float]] = _uniform_default()
    local_duration: Final['uniform'[float]] = _uniform_default()
    fps: Final['uniform'[float]] = _uniform_default()
    resolution: Final['uniform'['vec2']] = _uniform_default()
    mesh_size: Final['uniform'['vec2']] = _uniform_default()


@dataclass
class _GS_OUT:
    vUvs: 'vec2' = _default()
    sprite_idx: float = _default()


@dataclass
class _LayerFragInput:
    fs_in: Final['in_t'[_GS_OUT]] = _in_t_default()


@dataclass
class _frag(_LayerUniform):
    ...


@dataclass
class _VS_OUT:
    vUvs: 'vec2' = _default()
    sprite_idx: float = _default()


@dataclass
class _LayerPolyOutput:
    vs_out: 'out_t'[_VS_OUT] = _out_t_default()


@dataclass
class _poly(_LayerUniform):
    ...


_buffer_type = _frag | _poly

_NamedFnP = ParamSpec('_NamedFnP')
_NamedFnR = TypeVar('_NamedFnR')
_NamedFnStage = Literal['frag', 'poly', 'any']


def lib(stage: _NamedFnStage) -> Callable[[Callable[_NamedFnP, _NamedFnR]], Callable[_NamedFnP, _NamedFnR]]:
    def deco(f: Callable[_NamedFnP, _NamedFnR]) -> Callable[_NamedFnP, _NamedFnR]:
        def wrapper(_stage: _NamedFnStage = stage, *args: _NamedFnP.args, **kwargs: _NamedFnP.kwargs) -> _NamedFnR:
            return f(*args, **kwargs)
        return cast(Callable[_NamedFnP, _NamedFnR], wrapper)
    return deco


_TFragBuffer = TypeVar('_TFragBuffer', bound='_frag')
_TPolyBuffer = TypeVar('_TPolyBuffer', bound='_poly')


@overload
def entry(buffer_type: Type[_TFragBuffer]) -> Callable[['_NamedEntryFragFn'['_TFragBuffer']], _TEntryFnOpaque['_NamedEntryFragFn'['_TFragBuffer']]]:  # noqa: E501

    ...


@overload
def entry(buffer_type: Type[_TPolyBuffer]) -> Callable[['_NamedEntryPolyFn'['_TPolyBuffer']], _TEntryFnOpaque['_NamedEntryPolyFn'['_TPolyBuffer']]]:  # noqa: E501

    ...


def entry(buffer_type) -> Any:

    if issubclass(buffer_type, _frag):
        return _entry_frag()
    if issubclass(buffer_type, _poly):
        return _entry_poly()
    else:
        raise NotImplementedError()


def _entry_frag() -> Callable[['_NamedEntryFragFn'], _TEntryFnOpaque['_NamedEntryFragFn']]:
    def deco(f: Callable[_NamedFnP, _NamedFnR]) -> Callable[_NamedFnP, _NamedFnR]:
        def wrapper(_stage: _NamedFnStage = 'frag', *args: _NamedFnP.args, **kwargs: _NamedFnP.kwargs) -> _NamedFnR:
            return f(*args, **kwargs)
        return cast(Callable[_NamedFnP, _NamedFnR], wrapper)
    return deco  # type: ignore


def _entry_poly() -> Callable[['_NamedEntryPolyFn'], _TEntryFnOpaque['_NamedEntryPolyFn']]:
    def deco(f: Callable[_NamedFnP, _NamedFnR]) -> Callable[_NamedFnP, _NamedFnR]:
        def wrapper(_stage: _NamedFnStage = 'poly', *args: _NamedFnP.args, **kwargs: _NamedFnP.kwargs) -> _NamedFnR:
            return f(*args, **kwargs)
        return cast(Callable[_NamedFnP, _NamedFnR], wrapper)
    return deco  # type: ignore
