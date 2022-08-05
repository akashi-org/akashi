# pyright: reportPrivateUsage=false
from __future__ import annotations

__all__ = [
    "outer",
    "uint",
    "gl_FragCoord"
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

from _gl_common import uint
from _gl_vec import gvec2, gvec3, gvec4


vec4 = gvec4[float]
ivec4 = gvec4[int]
uvec4 = gvec4[uint]
bvec4 = gvec4[bool]

vec3 = gvec3[float]
ivec3 = gvec3[int]
uvec3 = gvec3[uint]
bvec3 = gvec3[bool]

vec2 = gvec2[float]
ivec2 = gvec2[int]
uvec2 = gvec2[uint]
bvec2 = gvec2[bool]

if TYPE_CHECKING:
    from .shader import _NamedEntryFragFn, _NamedEntryPolyFn, _TEntryFnOpaque


_T = TypeVar('_T')
_TNumber = TypeVar('_TNumber', int, float)


def outer(__expr: _T) -> _T:
    return __expr


''' Builtin Functions '''


def EmitVertex() -> None: ...


def EndPrimitive() -> None: ...


_TGenBType = TypeVar('_TGenBType', bool, 'bvec2', 'bvec3', 'bvec4')
_TGenIType = TypeVar('_TGenIType', int, 'ivec2', 'ivec3', 'ivec4')
_TGenUType = TypeVar('_TGenUType', uint, 'uvec2', 'uvec3', 'uvec4')
_TGenType = TypeVar('_TGenType', float, 'vec2', 'vec3', 'vec4')

# We should always start the definition with _TGenIType, _TGenUType over _TGenType


@overload
def abs(x: _TGenIType) -> _TGenIType: ...


@overload
def abs(x: _TGenType) -> _TGenType: ...


def abs(x) -> Any: ...


@overload
def sign(x: _TGenIType) -> _TGenIType: ...


@overload
def sign(x: _TGenType) -> _TGenType: ...


def sign(x) -> Any: ...


def length(x: _TGenType) -> float: ...


def distance(p0: _TGenType, p1: _TGenType) -> float: ...


def dot(x: _TGenType, y: _TGenType) -> float: ...


def cross(x: 'vec3', y: 'vec3') -> 'vec3': ...


def normalize(x: _TGenType) -> _TGenType: ...


def faceforward(N: _TGenType, I: _TGenType, Nref: _TGenType) -> _TGenType: ...  # noqa: E741


def reflect(I: _TGenType, N: _TGenType) -> _TGenType: ...  # noqa: E741


def refract(I: _TGenType, N: _TGenType, eta: float) -> _TGenType: ...  # noqa: E741


def sqrt(x: _TGenType) -> _TGenType: ...


def inversesqrt(x: _TGenType) -> _TGenType: ...


def pow(x: _TGenType, y: _TGenType) -> _TGenType: ...


def exp(x: _TGenType) -> _TGenType: ...


def exp2(x: _TGenType) -> _TGenType: ...


def log(x: _TGenType) -> _TGenType: ...


def log2(x: _TGenType) -> _TGenType: ...


def degrees(radians: _TGenType) -> _TGenType: ...


def radians(degrees: _TGenType) -> _TGenType: ...


def floor(x: _TGenType) -> _TGenType: ...


def ceil(x: _TGenType) -> _TGenType: ...


def fract(x: _TGenType) -> _TGenType: ...


@overload
def mod(x: _TGenType, y: float) -> _TGenType: ...


@overload
def mod(x: _TGenType, y: _TGenType) -> _TGenType: ...


def mod(x, y) -> Any: ...


def sin(angle: _TGenType) -> _TGenType: ...


def cos(angle: _TGenType) -> _TGenType: ...


def tan(angle: _TGenType) -> _TGenType: ...


def asin(x: _TGenType) -> _TGenType: ...


def acos(x: _TGenType) -> _TGenType: ...


def atan(a: _TGenType, b: Optional[_TGenType] = None) -> _TGenType: ...


@overload
def max(x: _TGenIType, y: _TGenIType) -> _TGenIType: ...


@overload
def max(x: _TGenIType, y: int) -> _TGenIType: ...


@overload
def max(x: _TGenUType, y: _TGenUType) -> _TGenUType: ...


@overload
def max(x: _TGenUType, y: uint) -> _TGenUType: ...


@overload
def max(x: _TGenType, y: _TGenType) -> _TGenType: ...


@overload
def max(x: _TGenType, y: float) -> _TGenType: ...


def max(x, y) -> Any: ...


@overload
def min(x: _TGenIType, y: _TGenIType) -> _TGenIType: ...


@overload
def min(x: _TGenIType, y: int) -> _TGenIType: ...


@overload
def min(x: _TGenUType, y: _TGenUType) -> _TGenUType: ...


@overload
def min(x: _TGenUType, y: uint) -> _TGenUType: ...


@overload
def min(x: _TGenType, y: _TGenType) -> _TGenType: ...


@overload
def min(x: _TGenType, y: float) -> _TGenType: ...


def min(x, y) -> Any: ...


@overload
def step(edge: _TGenType, x: _TGenType) -> _TGenType: ...


@overload
def step(edge: float, x: _TGenType) -> _TGenType: ...


def step(edge, x) -> Any: ...


@overload
def smoothstep(edge0: _TGenType, edge1: _TGenType, x: _TGenType) -> _TGenType: ...


@overload
def smoothstep(edge0: float, edge1: float, x: _TGenType) -> _TGenType: ...


def smoothstep(edge0, edge1, x) -> Any: ...


@overload
def mix(x: _TGenType, y: _TGenType, a: _TGenBType) -> _TGenType: ...


@overload
def mix(x: _TGenType, y: _TGenType, a: _TGenType) -> _TGenType: ...


@overload
def mix(x: _TGenType, y: _TGenType, a: float) -> _TGenType: ...


def mix(x, y, a) -> Any: ...


@overload
def clamp(x: _TGenUType, minVal: _TGenUType, maxVal: _TGenUType) -> _TGenUType: ...


@overload
def clamp(x: _TGenType, minVal: uint, maxVal: uint) -> _TGenUType: ...


@overload
def clamp(x: _TGenIType, minVal: _TGenIType, maxVal: _TGenIType) -> _TGenIType: ...


@overload
def clamp(x: _TGenIType, minVal: int, maxVal: int) -> _TGenIType: ...


@overload
def clamp(x: _TGenType, minVal: _TGenType, maxVal: _TGenType) -> _TGenType: ...


@overload
def clamp(x: _TGenType, minVal: float, maxVal: float) -> _TGenType: ...


def clamp(x, minVal, maxVal) -> Any: ...


def isnan(x: _TGenType) -> _TGenBType: ...


def isinf(x: _TGenType) -> _TGenBType: ...


@overload
def texture(sampler: gsampler1D['_TGenSamplerType'], P: float, Q: float = 0) -> gvec4['_TGenSamplerType']: ...


@overload
def texture(sampler: gsampler2D['_TGenSamplerType'], P: vec2, Q: float = 0) -> gvec4['_TGenSamplerType']: ...


@overload
def texture(sampler: gsampler3D['_TGenSamplerType'], P: vec3, Q: float = 0) -> gvec4['_TGenSamplerType']: ...


@overload
def texture(sampler: gsamplerCube['_TGenSamplerType'], P: vec3, Q: float = 0) -> gvec4['_TGenSamplerType']: ...


@overload
def texture(sampler: sampler1DShadow, P: vec3, Q: float = 0) -> float: ...


@overload
def texture(sampler: sampler2DShadow, P: vec3, Q: float = 0) -> float: ...


@overload
def texture(sampler: samplerCubeShadow, P: vec4, Q: float = 0) -> float: ...


@overload
def texture(sampler: gsampler1DArray['_TGenSamplerType'], P: vec2, Q: float = 0) -> gvec4['_TGenSamplerType']: ...


@overload
def texture(sampler: gsampler2DArray['_TGenSamplerType'], P: vec3, Q: float = 0) -> gvec4['_TGenSamplerType']: ...


@overload
def texture(sampler: gsamplerCubeArray['_TGenSamplerType'], P: vec4, Q: float = 0) -> gvec4['_TGenSamplerType']: ...


@overload
def texture(sampler: sampler1DArrayShadow, P: vec3, Q: float = 0) -> float: ...


@overload
def texture(sampler: sampler2DArrayShadow, P: vec4, Q: float = 0) -> float: ...


# Q is for arity issues. This argument should not be filled in.
@overload
def texture(sampler: gsampler2DRect['_TGenSamplerType'], P: vec2,
            Q: Any = None) -> gvec4['_TGenSamplerType']: ...


# Q is for arity issues. This argument should not be filled in.
@overload
def texture(sampler: sampler2DRectShadow, P: vec3,
            Q: Any = None) -> float: ...


@overload
def texture(sampler: gsamplerCubeArrayShadow['_TGenSamplerType'],
            P: vec4, Q: float) -> float: ...


def texture(sampler, P, Q: Any = 0) -> Any: ...


#
''' Function Parameter qualifiers '''
# https://www.khronos.org/opengl/wiki/Core_Language_(GLSL)#Parameters


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


''' Samplers '''


_TGenSamplerType = TypeVar('_TGenSamplerType', int, float, uint)


class gsampler1D(Generic[_TGenSamplerType]):
    ...


sampler1D = gsampler1D[float]
isampler1D = gsampler1D[int]
usampler1D = gsampler1D[uint]


class gsampler2D(Generic[_TGenSamplerType]):
    ...


sampler2D = gsampler2D[float]
isampler2D = gsampler2D[int]
usampler2D = gsampler2D[uint]


class gsampler3D(Generic[_TGenSamplerType]):
    ...


sampler3D = gsampler3D[float]
isampler3D = gsampler3D[int]
usampler3D = gsampler3D[uint]


class gsamplerCube(Generic[_TGenSamplerType]):
    ...


samplerCube = gsamplerCube[float]
isamplerCube = gsamplerCube[int]
usamplerCube = gsamplerCube[uint]


class gsampler2DRect(Generic[_TGenSamplerType]):
    ...


sampler2DRect = gsampler2DRect[float]
isampler2DRect = gsampler2DRect[int]
usampler2DRect = gsampler2DRect[uint]


class gsampler1DArray(Generic[_TGenSamplerType]):
    ...


sampler1DArray = gsampler1DArray[float]
isampler1DArray = gsampler1DArray[int]
usampler1DArray = gsampler1DArray[uint]


class gsampler2DArray(Generic[_TGenSamplerType]):
    ...


sampler2DArray = gsampler2DArray[float]
isampler2DArray = gsampler2DArray[int]
usampler2DArray = gsampler2DArray[uint]


class gsamplerCubeArray(Generic[_TGenSamplerType]):
    ...


samplerCubeArray = gsamplerCubeArray[float]
isamplerCubeArray = gsamplerCubeArray[int]
usamplerCubeArray = gsamplerCubeArray[uint]


class gsamplerBuffer(Generic[_TGenSamplerType]):
    ...


samplerBuffer = gsamplerBuffer[float]
isamplerBuffer = gsamplerBuffer[int]
usamplerBuffer = gsamplerBuffer[uint]


class gsampler2DMS(Generic[_TGenSamplerType]):
    ...


sampler2DMS = gsampler2DMS[float]
isampler2DMS = gsampler2DMS[int]
usampler2DMS = gsampler2DMS[uint]


class gsampler2DMSArray(Generic[_TGenSamplerType]):
    ...


sampler2DMSArray = gsampler2DMSArray[float]
isampler2DMSArray = gsampler2DMSArray[int]
usampler2DMSArray = gsampler2DMSArray[uint]

''' Shadow samplers '''


class sampler1DShadow():
    ...


class sampler2DShadow():
    ...


class samplerCubeShadow():
    ...


class sampler2DRectShadow():
    ...


class sampler1DArrayShadow():
    ...


class sampler2DArrayShadow():
    ...


class gsamplerCubeArrayShadow(Generic[_TGenSamplerType]):
    ...


samplerCubeArrayShadow = gsamplerCubeArrayShadow[float]
isamplerCubeArrayShadow = gsamplerCubeArrayShadow[int]
usamplerCubeArrayShadow = gsamplerCubeArrayShadow[uint]


''' Built-in Variables '''

# Geometry Shader Special Variables
'''
in gl_PerVertex {
    vec4  gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
} gl_in[];
in int gl_PrimitiveIDIn;
in int gl_InvocationID;

out gl_PerVertex {
    vec4  gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
};
out int gl_PrimitiveID;
out int gl_Layer;
out int gl_ViewportIndex;
'''

# Fragment Shader Special Variables

'''
in  vec4  gl_FragCoord;
in  bool  gl_FrontFacing;
in  float gl_ClipDistance[];
in  vec2  gl_PointCoord;
in  int   gl_PrimitiveID;
in  int   gl_SampleID;
in  vec2  gl_SamplePosition;
in  int   gl_SampleMaskIn[];
in  int   gl_Layer;
in  int   gl_ViewportIndex;

out float gl_FragDepth;
out int   gl_SampleMask[];
'''

gl_FragCoord = vec4(1, 1, 1, 1)  # temporary

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
