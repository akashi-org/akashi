from __future__ import annotations

__all__ = [
    "eval",
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
    TYPE_CHECKING
)

if TYPE_CHECKING:
    from akashi_core.pysl.shader import NamedEntryFragFn, NamedEntryPolyFn, TEntryFnOpaque


_T = TypeVar('_T')
_TNumber = TypeVar('_TNumber', int, float)

_NamedFnP = ParamSpec('_NamedFnP')
_NamedFnR = TypeVar('_NamedFnR')
_NamedFnStage = Literal['frag', 'poly', 'any']


def fn(stage: _NamedFnStage) -> Callable[[Callable[_NamedFnP, _NamedFnR]], Callable[_NamedFnP, _NamedFnR]]:
    def deco(f: Callable[_NamedFnP, _NamedFnR]) -> Callable[_NamedFnP, _NamedFnR]:
        def wrapper(_stage: _NamedFnStage = stage, *args: _NamedFnP.args, **kwargs: _NamedFnP.kwargs) -> _NamedFnR:
            return f(*args, **kwargs)
        return wrapper
    return deco


# [TODO] Can we merge entry_*() decorators by using typing.overload?


def entry_frag() -> Callable[['NamedEntryFragFn'], TEntryFnOpaque['NamedEntryFragFn']]:
    def deco(f: Callable[_NamedFnP, _NamedFnR]) -> Callable[_NamedFnP, _NamedFnR]:
        def wrapper(_stage: _NamedFnStage = 'frag', *args: _NamedFnP.args, **kwargs: _NamedFnP.kwargs) -> _NamedFnR:
            return f(*args, **kwargs)
        return wrapper
    return deco  # type: ignore


def entry_poly() -> Callable[['NamedEntryPolyFn'], TEntryFnOpaque['NamedEntryPolyFn']]:
    def deco(f: Callable[_NamedFnP, _NamedFnR]) -> Callable[_NamedFnP, _NamedFnR]:
        def wrapper(_stage: _NamedFnStage = 'poly', *args: _NamedFnP.args, **kwargs: _NamedFnP.kwargs) -> _NamedFnR:
            return f(*args, **kwargs)
        return wrapper
    return deco  # type: ignore


def eval(_expr: _T) -> _T:
    return _expr


# [TODO] should we wrap it with NewType?
uint = int

''' Builtin Functions '''


def EmitVertex() -> None: ...


def EndPrimitive() -> None: ...


# _TGenBType = TypeVar('_TGenBType', bool, 'bvec2', 'bvec3', 'bvec4')
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

# [TODO] mix has several overloads with _TGenBType.
# Maybe we should define these later.


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


def texture(sampler, P, Q=0) -> Any: ...


#
''' Function Parameter qualifiers '''
# https://www.khronos.org/opengl/wiki/Core_Language_(GLSL)#Parameters


@dataclass
class in_p(Generic[_T]):
    value: _T


@dataclass
class out_p(Generic[_T]):
    value: _T


@dataclass
class inout_p(Generic[_T]):
    value: _T


@dataclass
class __2d_prop(Generic[_TNumber]):

    __value: list[_TNumber]

    @property
    def x(self) -> _TNumber:
        return self.__value[0]

    @x.setter
    def x(self, item: _TNumber):
        self.__value[0] = item

    @property
    def y(self) -> _TNumber:
        return self.__value[1]

    @y.setter
    def y(self, item: _TNumber):
        self.__value[1] = item

    @property
    def r(self) -> _TNumber:
        return self.__value[0]

    @r.setter
    def r(self, item: _TNumber):
        self.__value[0] = item

    @property
    def g(self) -> _TNumber:
        return self.__value[1]

    @g.setter
    def g(self, item: _TNumber):
        self.__value[1] = item

    @property
    def xy(self) -> gvec2[_TNumber]:
        return gvec2[_TNumber](self.__value[0], self.__value[1])

    @xy.setter
    def xy(self, item: gvec2[_TNumber]):
        self.__value[0] = item.x
        self.__value[1] = item.y

    @property
    def rg(self) -> gvec2[_TNumber]:
        return gvec2[_TNumber](self.__value[0], self.__value[1])

    @rg.setter
    def rg(self, item: gvec2[_TNumber]):
        self.__value[0] = item.x
        self.__value[1] = item.y


@dataclass
class __3d_prop(__2d_prop[_TNumber], Generic[_TNumber]):

    __value: list[_TNumber]

    @property
    def z(self) -> _TNumber:
        return self.__value[2]

    @z.setter
    def z(self, item: _TNumber):
        self.__value[2] = item

    @property
    def b(self) -> _TNumber:
        return self.__value[2]

    @b.setter
    def b(self, item: _TNumber):
        self.__value[2] = item

    @property
    def yz(self) -> gvec2[_TNumber]:
        return gvec2[_TNumber](self.__value[1], self.__value[2])

    @yz.setter
    def yz(self, item: gvec2[_TNumber]):
        self.__value[1] = item.x
        self.__value[2] = item.y

    @property
    def gb(self) -> gvec2[_TNumber]:
        return gvec2[_TNumber](self.__value[1], self.__value[2])

    @gb.setter
    def gb(self, item: gvec2[_TNumber]):
        self.__value[1] = item.x
        self.__value[2] = item.y

    @property
    def xyz(self) -> gvec3[_TNumber]:
        return gvec3[_TNumber](self.__value[0], self.__value[1], self.__value[2])

    @xyz.setter
    def xyz(self, item: gvec3[_TNumber]):
        self.__value[0] = item.x
        self.__value[1] = item.y
        self.__value[2] = item.z

    @property
    def rgb(self) -> gvec3[_TNumber]:
        return gvec3[_TNumber](self.__value[0], self.__value[1], self.__value[2])

    @rgb.setter
    def rgb(self, item: gvec3[_TNumber]):
        self.__value[0] = item.x
        self.__value[1] = item.y
        self.__value[2] = item.z


@dataclass
class __4d_prop(__3d_prop[_TNumber], Generic[_TNumber]):

    __value: list[_TNumber]

    @property
    def w(self) -> _TNumber:
        return self.__value[3]

    @w.setter
    def w(self, item: _TNumber):
        self.__value[3] = item

    @property
    def a(self) -> _TNumber:
        return self.__value[3]

    @a.setter
    def a(self, item: _TNumber):
        self.__value[3] = item

    @property
    def zw(self) -> gvec2[_TNumber]:
        return gvec2[_TNumber](self.__value[2], self.__value[3])

    @zw.setter
    def zw(self, item: gvec2[_TNumber]):
        self.__value[2] = item.x
        self.__value[3] = item.y

    @property
    def ba(self) -> gvec2[_TNumber]:
        return gvec2[_TNumber](self.__value[2], self.__value[3])

    @ba.setter
    def ba(self, item: gvec2[_TNumber]):
        self.__value[2] = item.x
        self.__value[3] = item.y

    @property
    def yzw(self) -> gvec3[_TNumber]:
        return gvec3[_TNumber](self.__value[1], self.__value[2], self.__value[3])

    @yzw.setter
    def yzw(self, item: gvec3[_TNumber]):
        self.__value[1] = item.x
        self.__value[2] = item.y
        self.__value[3] = item.z

    @property
    def gba(self) -> gvec3[_TNumber]:
        return gvec3[_TNumber](self.__value[1], self.__value[2], self.__value[3])

    @gba.setter
    def gba(self, item: gvec3[_TNumber]):
        self.__value[1] = item.x
        self.__value[2] = item.y
        self.__value[3] = item.z

    @property
    def xyzw(self) -> gvec4[_TNumber]:
        return gvec4[_TNumber](self.__value[0], self.__value[1], self.__value[2], self.__value[3])

    @xyzw.setter
    def xyzw(self, item: gvec4[_TNumber]):
        self.__value[0] = item.x
        self.__value[1] = item.y
        self.__value[2] = item.z
        self.__value[3] = item.w

    @property
    def rgba(self) -> gvec4[_TNumber]:
        return gvec4[_TNumber](self.__value[0], self.__value[1], self.__value[2], self.__value[3])

    @rgba.setter
    def rgba(self, item: gvec4[_TNumber]):
        self.__value[0] = item.x
        self.__value[1] = item.y
        self.__value[2] = item.z
        self.__value[3] = item.w


@dataclass
class gvec4(__4d_prop[_TNumber], Generic[_TNumber]):

    __value: list[_TNumber]

    def __init__(self, a1: _TNumber, a2: _TNumber, a3: _TNumber, a4: _TNumber):
        self.__value = [a1, a2, a3, a4]

    def __getitem__(self, idx: int):
        return self.__value[idx]

    def __setitem__(self, idx: int, item: _TNumber):
        self.__value[idx] = item

    def __len__(self):
        return len(self.__value)

    @overload
    def __add__(self, other: _TNumber) -> gvec4[_TNumber]:
        ...

    @overload
    def __add__(self, other: gvec4[_TNumber]) -> gvec4[_TNumber]:
        ...

    def __add__(self, other) -> gvec4[_TNumber]:
        ...

    @overload
    def __sub__(self, other: _TNumber) -> gvec4[_TNumber]:
        ...

    @overload
    def __sub__(self, other: gvec4[_TNumber]) -> gvec4[_TNumber]:
        ...

    def __sub__(self, other) -> gvec4[_TNumber]:
        ...

    @overload
    def __mul__(self: gvec4[_TNumber], other: _TNumber) -> gvec4[_TNumber]:
        ...

    @overload
    def __mul__(self: gvec4[_TNumber], other: gvec4[_TNumber]) -> gvec4[_TNumber]:
        ...

    def __mul__(self: gvec4[_TNumber], other: Any) -> gvec4[_TNumber]:
        if isinstance(other, gvec4):
            return gvec4(*[a * b for a, b in zip(self._value(), other._value())])
        elif isinstance(other, int) or isinstance(other, float):
            return gvec4(*cast(tuple[_TNumber, _TNumber, _TNumber, _TNumber], (a * other for a in self._value())))
        else:
            raise Exception('__mul__ is not supported')

    @overload
    def __truediv__(self, other: _TNumber) -> gvec4[_TNumber]:
        ...

    @overload
    def __truediv__(self, other: gvec4[_TNumber]) -> gvec4[_TNumber]:
        ...

    def __truediv__(self, other) -> gvec4[_TNumber]:
        ...

    def _value(self) -> list[_TNumber]:
        return self.__value


vec4 = gvec4[float]
ivec4 = gvec4[int]
uvec4 = gvec4[uint]
# bvec4 = gvec4[bool]


@dataclass
class gvec3(__3d_prop[_TNumber], Generic[_TNumber]):

    __value: list[_TNumber]

    def __init__(self, a1: _TNumber, a2: _TNumber, a3: _TNumber):
        self.__value = [a1, a2, a3]

    def __getitem__(self, idx: int):
        return self.__value[idx]

    def __setitem__(self, idx: int, item: _TNumber):
        self.__value[idx] = item

    def __len__(self):
        return len(self.__value)

    @overload
    def __add__(self, other: _TNumber) -> gvec3[_TNumber]:
        ...

    @overload
    def __add__(self, other: gvec3[_TNumber]) -> gvec3[_TNumber]:
        ...

    def __add__(self, other) -> gvec3[_TNumber]:
        ...

    @overload
    def __sub__(self, other: _TNumber) -> gvec3[_TNumber]:
        ...

    @overload
    def __sub__(self, other: gvec3[_TNumber]) -> gvec3[_TNumber]:
        ...

    def __sub__(self, other) -> gvec3[_TNumber]:
        ...

    @overload
    def __mul__(self, other: _TNumber) -> gvec3[_TNumber]:
        ...

    @overload
    def __mul__(self, other: gvec3[_TNumber]) -> gvec3[_TNumber]:
        ...

    def __mul__(self, other) -> gvec3[_TNumber]:
        ...

    @overload
    def __truediv__(self, other: _TNumber) -> gvec3[_TNumber]:
        ...

    @overload
    def __truediv__(self, other: gvec3[_TNumber]) -> gvec3[_TNumber]:
        ...

    def __truediv__(self, other) -> gvec3[_TNumber]:
        ...


@dataclass
class gvec2(__2d_prop[_TNumber], Generic[_TNumber]):

    __value: list[_TNumber]

    def __init__(self, a1: _TNumber, a2: _TNumber):
        self.__value = [a1, a2]

    def __getitem__(self, idx: int):
        return self.__value[idx]

    def __setitem__(self, idx: int, item: _TNumber):
        self.__value[idx] = item

    def __len__(self):
        return len(self.__value)

    @overload
    def __add__(self, other: _TNumber) -> gvec2[_TNumber]:
        ...

    @overload
    def __add__(self, other: gvec2[_TNumber]) -> gvec2[_TNumber]:
        ...

    def __add__(self, other) -> gvec2[_TNumber]:
        ...

    @overload
    def __sub__(self, other: _TNumber) -> gvec2[_TNumber]:
        ...

    @overload
    def __sub__(self, other: gvec2[_TNumber]) -> gvec2[_TNumber]:
        ...

    def __sub__(self, other) -> gvec2[_TNumber]:
        ...

    @overload
    def __mul__(self, other: _TNumber) -> gvec2[_TNumber]:
        ...

    @overload
    def __mul__(self, other: gvec2[_TNumber]) -> gvec2[_TNumber]:
        ...

    def __mul__(self, other) -> gvec2[_TNumber]:
        ...

    @overload
    def __truediv__(self, other: _TNumber) -> gvec2[_TNumber]:
        ...

    @overload
    def __truediv__(self, other: gvec2[_TNumber]) -> gvec2[_TNumber]:
        ...

    def __truediv__(self, other) -> gvec2[_TNumber]:
        ...


vec3 = gvec3[float]
ivec3 = gvec3[int]
uvec3 = gvec3[uint]

vec2 = gvec2[float]
ivec2 = gvec2[int]
uvec2 = gvec2[uint]


@dataclass(frozen=True)
class uniform(Generic[_T]):
    value: _T

    @staticmethod
    def default() -> Any:
        return uniform[_T](None)


@dataclass(frozen=True)
class in_t(Generic[_T]):
    value: _T

    @staticmethod
    def default() -> Any:
        return in_t[_T](None)


@dataclass(frozen=True)
class out_t(Generic[_T]):
    value: _T

    @staticmethod
    def default() -> Any:
        return out_t[_T](None)


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
