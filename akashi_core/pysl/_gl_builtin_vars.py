# pyright: reportPrivateUsage=false
from __future__ import annotations

from typing import TYPE_CHECKING

from akashi_core.pysl._gl_vec import vec4

if TYPE_CHECKING:
    from akashi_core.pysl._gl import in_t


''' 
  7.1 Built-In Language Variables
'''

# Vertex Shader Special Variables
'''
in int gl_VertexID;
in int gl_InstanceID;

out gl_PerVertex {
    vec4  gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
} gl_out[];
'''

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

gl_FragCoord: 'in_t'[vec4] = vec4(1)


''' 
    7.3 Built-In Constants 
'''

''' 
    7.4 Built-In Uniform State
'''

'''
struct gl_DepthRangeParameters {
    float near;
    float far;
    float diff;
};

uniform gl_DepthRangeParameters gl_DepthRange;
uniform int gl_NumSamples;
'''
