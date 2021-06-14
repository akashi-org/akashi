import unittest
from akashi_core import (
    EntryShader,
    LibShader
)


class TestShader(unittest.TestCase):

    def test_frag(self) -> None:

        s1 = EntryShader(
            type='frag',
            layer_type='video',
            src='''
                vec4 invert_filter(in vec4 base) { return vec4(vec3(1.0 - base), base.a); }
                void frag_main(inout vec4 _fragColor){
                    _fragColor = invert_filter(_fragColor);
                }
            '''
        )

        print(s1._assemble())

    def test_frag_include(self) -> None:

        la1 = LibShader(
            type='any',
            src='''
                float rand(vec2 co) { return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453); }
        ''')

        lf1 = LibShader(
            type='frag',
            src='''
                vec4 edge_filter(in vec4 base) { return vec4(vec3(fwidth(length(base.rgb))), base.a); }
        ''')

        s1 = EntryShader(
            type='frag',
            layer_type='video',
            includes=(la1, lf1),
            src='''
                vec4 invert_filter(in vec4 base) { return vec4(vec3(1.0 - base), base.a); }
                void frag_main(inout vec4 _fragColor){
                    _fragColor = invert_filter(_fragColor);
                }
            '''
        )
        print(s1._assemble())

    def test_geom(self) -> None:

        s1 = EntryShader(
            layer_type='video',
            type='geom',
            src='''
                layout(triangles) in;
                layout(triangle_strip, max_vertices = 3) out;
                uniform float time;
                in VS_OUT {
                    vec2 vLumaUvs;
                    vec2 vChromaUvs;
                }
                gs_in[];
                out GS_OUT {
                    vec2 vLumaUvs;
                    vec2 vChromaUvs;
                }
                gs_out;

                void pass_through() {
                    for (int i = 0; i < 3; i++) {
                        gs_out.vLumaUvs = gs_in[i].vLumaUvs;
                        gs_out.vChromaUvs = gs_in[i].vChromaUvs;
                        gl_Position = gl_in[i].gl_Position;
                        EmitVertex();
                    }
                    EndPrimitive();
                }
                void main() { pass_through(); }
            '''
        )

        print(s1._assemble())

    def test_lib_include(self) -> None:

        la1 = LibShader(
            type='any',
            src='''
                float rand(vec2 co) { return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453); }
        ''')

        lf1 = LibShader(
            type='frag',
            includes=(la1,),
            src='''
                vec4 edge_filter(in vec4 base) { return vec4(vec3(fwidth(length(base.rgb))), base.a); }
        ''')

        print(lf1._assemble())


if __name__ == "__main__":
    unittest.main()
