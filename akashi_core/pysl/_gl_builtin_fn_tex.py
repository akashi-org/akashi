# pyright: reportPrivateUsage=false
from __future__ import annotations

from typing import overload, Any, TYPE_CHECKING

if TYPE_CHECKING:
    from akashi_core.pysl._gl_vec import gvec4, vec4, vec3, vec2
    from akashi_core.pysl._gl_sampler import (
        _TGenSamplerType,
        gsampler1D, gsampler2D, gsampler3D, gsampler1DArray, gsampler2DArray,
        sampler1DShadow, sampler2DShadow, sampler1DArrayShadow, sampler2DArrayShadow,
        gsamplerCube, gsamplerCubeArray, samplerCubeShadow, gsamplerCubeArrayShadow,
        gsampler2DMS, gsampler2DRect, sampler2DRectShadow
    )

''' 
    8.9 Texture Functions
'''

# [TODO] impl other functions later


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
