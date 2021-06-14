from akashi_core import (
    root,
    scene,
    atom,
    video,
    VideoLayerParams,
    Second,
    akexport_elem,
    from_relpath,
    EntryShader,
    LibShader
)
from akconf import config
from layers.message_layer import message_layer

(WIDTH, HEIGHT) = config().video.resolution


def invert_filter():
    return LibShader(
        type='frag',
        src='''
            vec4 invert_filter(in vec4 base) { return vec4(vec3(1.0 - base), base.a); }
        '''
    )


def edge_filter():
    return LibShader(
        type='frag',
        src='''
            vec4 edge_filter(in vec4 base) {
                return vec4(vec3(fwidth(length(base.rgb))), base.a);
            }
        '''
    )


@akexport_elem()
def main():
    return root({}, [
        scene({}, [
            atom({}, [
                # message_layer(int(WIDTH / 2), int(HEIGHT / 2), Second(5)),
                video(init=VideoLayerParams(
                    src=from_relpath(__file__, 'assets/river.mp4'),
                    x=int(WIDTH / 2),
                    y=int(HEIGHT / 2),
                    begin=Second(0),
                    end=Second(10),
                    frag=EntryShader(
                        type='frag',
                        layer_type='video',
                        includes=(invert_filter(), edge_filter()),
                        src='''
                            void frag_main(inout vec4 _fragColor) {
                                // _fragColor = invert_filter(_fragColor);
                                // _fragColor = edge_filter(_fragColor);
                            }
                        '''
                    )
                ))
            ])
        ])

    ])
