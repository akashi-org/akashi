from akashi_core import (
    root,
    scene,
    atom,
    video,
    VideoLayerParams,
    Second,
    akexport_elem,
    from_relpath
)
from akconf import config
from layers.message_layer import message_layer

(WIDTH, HEIGHT) = config().video.resolution


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
                    frag_path=from_relpath(__file__, './video.frag')
                ))
            ])
        ])

    ])
