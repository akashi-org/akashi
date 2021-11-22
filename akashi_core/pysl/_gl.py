from __future__ import annotations
from dataclasses import dataclass
from typing import TypeVar, Generic, overload, cast, Any, Optional, Literal

_T = TypeVar('_T')
_TNumber = TypeVar('_TNumber', int, float)


def __gl_func(fn):
    def inner(*args, **kwargs):
        fn(*args, **kwargs)
    return inner


def __gl_instance_method(fn):
    def inner(*args, **kwargs):
        fn(*args, **kwargs)
    return inner


struct = dataclass
test_func = __gl_func
func = staticmethod
method = __gl_instance_method
module = dataclass


uint = int

''' Builtin Functions '''


@staticmethod
def EmitVertex() -> None: ...


@staticmethod
def EndPrimitive() -> None: ...


# _TGenBType = TypeVar('_TGenBType', bool, 'bvec2', 'bvec3', 'bvec4')
_TGenIType = TypeVar('_TGenIType', int, 'ivec2', 'ivec3', 'ivec4')
_TGenUType = TypeVar('_TGenUType', uint, 'uvec2', 'uvec3', 'uvec4')
_TGenType = TypeVar('_TGenType', float, 'vec2', 'vec3', 'vec4')

# We should always start the definition with _TGenIType, _TGenUType over _TGenType


@overload
@staticmethod
def abs(x: _TGenIType) -> _TGenIType: ...


@overload
@staticmethod
def abs(x: _TGenType) -> _TGenType: ...


@staticmethod
def abs(x) -> Any: ...


@overload
@staticmethod
def sign(x: _TGenIType) -> _TGenIType: ...


@overload
@staticmethod
def sign(x: _TGenType) -> _TGenType: ...


@staticmethod
def sign(x) -> Any: ...


@staticmethod
def length(x: _TGenType) -> float: ...


@staticmethod
def distance(p0: _TGenType, p1: _TGenType) -> float: ...


@staticmethod
def dot(x: _TGenType, y: _TGenType) -> float: ...


@staticmethod
def cross(x: 'vec3', y: 'vec3') -> 'vec3': ...


@staticmethod
def normalize(x: _TGenType) -> _TGenType: ...


@staticmethod
def faceforward(N: _TGenType, I: _TGenType, Nref: _TGenType) -> _TGenType: ...  # noqa: E741


@staticmethod
def reflect(I: _TGenType, N: _TGenType) -> _TGenType: ...  # noqa: E741


@staticmethod
def refract(I: _TGenType, N: _TGenType, eta: float) -> _TGenType: ...  # noqa: E741


@staticmethod
def sqrt(x: _TGenType) -> _TGenType: ...


@staticmethod
def inversesqrt(x: _TGenType) -> _TGenType: ...


@staticmethod
def pow(x: _TGenType, y: _TGenType) -> _TGenType: ...


@staticmethod
def exp(x: _TGenType) -> _TGenType: ...


@staticmethod
def exp2(x: _TGenType) -> _TGenType: ...


@staticmethod
def log(x: _TGenType) -> _TGenType: ...


@staticmethod
def log2(x: _TGenType) -> _TGenType: ...


@staticmethod
def degrees(radians: _TGenType) -> _TGenType: ...


@staticmethod
def radians(degrees: _TGenType) -> _TGenType: ...


@staticmethod
def floor(x: _TGenType) -> _TGenType: ...


@staticmethod
def ceil(x: _TGenType) -> _TGenType: ...


@staticmethod
def fract(x: _TGenType) -> _TGenType: ...


@overload
@staticmethod
def mod(x: _TGenType, y: float) -> _TGenType: ...


@overload
@staticmethod
def mod(x: _TGenType, y: _TGenType) -> _TGenType: ...


@staticmethod
def mod(x, y) -> Any: ...


@staticmethod
def sin(angle: _TGenType) -> _TGenType: ...


@staticmethod
def cos(angle: _TGenType) -> _TGenType: ...


@staticmethod
def tan(angle: _TGenType) -> _TGenType: ...


@staticmethod
def asin(x: _TGenType) -> _TGenType: ...


@staticmethod
def acos(x: _TGenType) -> _TGenType: ...


@staticmethod
def atan(a: _TGenType, b: Optional[_TGenType] = None) -> _TGenType: ...


@overload
@staticmethod
def max(x: _TGenIType, y: _TGenIType) -> _TGenIType: ...


@overload
@staticmethod
def max(x: _TGenIType, y: int) -> _TGenIType: ...


@overload
@staticmethod
def max(x: _TGenUType, y: _TGenUType) -> _TGenUType: ...


@overload
@staticmethod
def max(x: _TGenUType, y: uint) -> _TGenUType: ...


@overload
@staticmethod
def max(x: _TGenType, y: _TGenType) -> _TGenType: ...


@overload
@staticmethod
def max(x: _TGenType, y: float) -> _TGenType: ...


@staticmethod
def max(x, y) -> Any: ...


@overload
@staticmethod
def min(x: _TGenIType, y: _TGenIType) -> _TGenIType: ...


@overload
@staticmethod
def min(x: _TGenIType, y: int) -> _TGenIType: ...


@overload
@staticmethod
def min(x: _TGenUType, y: _TGenUType) -> _TGenUType: ...


@overload
@staticmethod
def min(x: _TGenUType, y: uint) -> _TGenUType: ...


@overload
@staticmethod
def min(x: _TGenType, y: _TGenType) -> _TGenType: ...


@overload
@staticmethod
def min(x: _TGenType, y: float) -> _TGenType: ...


@staticmethod
def min(x, y) -> Any: ...


@overload
@staticmethod
def step(edge: _TGenType, x: _TGenType) -> _TGenType: ...


@overload
@staticmethod
def step(edge: float, x: _TGenType) -> _TGenType: ...


@staticmethod
def step(edge, x) -> Any: ...


@overload
@staticmethod
def smoothstep(edge0: _TGenType, edge1: _TGenType, x: _TGenType) -> _TGenType: ...


@overload
@staticmethod
def smoothstep(edge0: float, edge1: float, x: _TGenType) -> _TGenType: ...


@staticmethod
def smoothstep(edge0, edge1, x) -> Any: ...

# [TODO] mix has several overloads with _TGenBType.
# Maybe we should define these later.


@overload
@staticmethod
def mix(x: _TGenType, y: _TGenType, a: _TGenType) -> _TGenType: ...


@overload
@staticmethod
def mix(x: _TGenType, y: _TGenType, a: float) -> _TGenType: ...


@staticmethod
def mix(x, y, a) -> Any: ...


@overload
@staticmethod
def clamp(x: _TGenUType, minVal: _TGenUType, maxVal: _TGenUType) -> _TGenUType: ...


@overload
@staticmethod
def clamp(x: _TGenType, minVal: uint, maxVal: uint) -> _TGenUType: ...


@overload
@staticmethod
def clamp(x: _TGenIType, minVal: _TGenIType, maxVal: _TGenIType) -> _TGenIType: ...


@overload
@staticmethod
def clamp(x: _TGenIType, minVal: int, maxVal: int) -> _TGenIType: ...


@overload
@staticmethod
def clamp(x: _TGenType, minVal: _TGenType, maxVal: _TGenType) -> _TGenType: ...


@overload
@staticmethod
def clamp(x: _TGenType, minVal: float, maxVal: float) -> _TGenType: ...


@staticmethod
def clamp(x, minVal, maxVal) -> Any: ...


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
    def __mul__(self: vec4[_TNumber], other: _TNumber) -> gvec4[_TNumber]:
        ...

    @overload
    def __mul__(self: vec4[_TNumber], other: gvec4[_TNumber]) -> gvec4[_TNumber]:
        ...

    def __mul__(self: vec4[_TNumber], other) -> gvec4[_TNumber]:
        if isinstance(other, gvec4[_TNumber]):
            return gvec4(*[a * b for a, b in zip(self._value(), other._value())])
        elif isinstance(other, int) or isinstance(other, float):
            return gvec4(*[a * other for a in self._value()])
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


@dataclass
class dynamic(Generic[_T]):
    value: _T


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


class samplerCubeArrayShadow():
    ...


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
