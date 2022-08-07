# pyright: reportPrivateUsage=false
from __future__ import annotations

from typing import TypeVar, Generic, TYPE_CHECKING

from ._gl_common import uint


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
