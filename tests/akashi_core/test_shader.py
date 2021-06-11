import unittest
from akashi_core import (
    FragShader,
    GeomShader,
    AnyShader
)


class TestShader(unittest.TestCase):

    def test_frag(self) -> None:

        s1 = FragShader(
            entry='''
                _fragColor = invert_filter(_fragColor);
            ''',
            src='''
                vec4 invert_filter(in vec4 base) { return vec4(vec3(1.0 - base), base.a); }
            '''
        )

        print(s1._assemble())

    def test_frag_module(self) -> None:

        s1 = FragShader(
            entry='''
                {
                  _fragColor = invert_filter(_fragColor);
                }
            ''',
            src='''
                vec4 invert_filter(in vec4 base) { return vec4(vec3(1.0 - base), base.a); }
            '''
        )

        a1 = AnyShader('''
            float rand(vec2 co) { return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453); }
        '''
                       )

        s2 = FragShader(
            entry='''
                _fragColor = edge_filter(_fragColor);
            ''',
            includes=[a1],
            src='''
                vec4 edge_filter(in vec4 base) {
                    return vec4(vec3(fwidth(length(base.rgb))), base.a);
                }
            '''
        )

        for mod_src in (s1 >> s2)._assemble():
            print(mod_src)
            print('-' * 20)

    def test_geom(self) -> None:

        s1 = GeomShader(
            entry='''
                pass_through();
            ''',
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

            '''
        )

        print(s1._assemble())


if __name__ == "__main__":
    unittest.main()
