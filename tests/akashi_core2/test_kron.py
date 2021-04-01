import unittest
from akashi_core2 import (
    root,
    scene,
    atom,
    video,
    audio,
    text,
    VideoLayerParams,
    AudioLayerParams,
    TextLayerParams,
    KronArgs,
    Second
)
from dataclasses import replace


class TestKron(unittest.TestCase):

    def test1_root(self) -> None:

        target_root = root({}, [
            scene({'path': 'path1'}, [
                atom({}, []),
                atom({'duration': Second(12)}, [
                    video(VideoLayerParams(
                        begin=Second(0), end=Second(10), x=0, y=0, src='/to/path/video.mp4'
                    )),
                    text(TextLayerParams(
                        begin=Second(0), end=Second(10), x=0, y=0, text='Hello, World!'
                    )),
                ]),
            ]),
        ])

        print(target_root)

    def test2_scene(self) -> None:

        target_scene = scene({'path': 'path1'}, [
            atom({'duration': Second(5)}, []),
            atom({}, [
                video(VideoLayerParams(
                    begin=Second(0), end=Second(10), x=0, y=0, src='/to/path/video.mp4'
                )),
                text(TextLayerParams(
                    begin=Second(0), end=Second(20), x=0, y=0, text='Hello, World!'
                ))
            ])
        ])

        print(target_scene)

    def test3_atom(self) -> None:

        target_atom = atom({}, [
            video(VideoLayerParams(
                begin=Second(0), end=Second(10), x=0, y=0, src='/to/path/video.mp4'
            )),
            text(TextLayerParams(
                begin=Second(0), end=Second(20), x=0, y=0, text='Hello, World!'
            ))
        ])

        print(target_atom)

    def test4_layer(self) -> None:

        def init():
            return VideoLayerParams(
                begin=Second(0), end=Second(10), x=100, y=0, src='/to/path/video.mp4'
            )

        def update(kronArgs: KronArgs, params: VideoLayerParams):
            return replace(params, x=params.x + 10)

        target_layer = video(init=init(), update=update)

        print(target_layer)


if __name__ == "__main__":
    unittest.main()
