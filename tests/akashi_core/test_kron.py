import unittest
from akashi_core import (
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
from akashi_core.kron import LayerReceiver, KronReceiver
from akashi_core.kron_utils import get_atom_profiles
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

        target_root(KronReceiver(
            get_children=lambda children: print(children),
            get_finalized_params=lambda params: print(params),
            get_frame_cnt=lambda frame_cnt: print(frame_cnt),
            get_duration=lambda duration: print(duration, float(duration)),
        ))(KronArgs(Second(5), 30))

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

        target_scene(KronReceiver(
            get_children=lambda children: print(children),
            get_finalized_params=lambda params: print(params),
            get_frame_cnt=lambda frame_cnt: print(frame_cnt),
            get_duration=lambda duration: print(duration, float(duration)),
        ))(KronArgs(Second(5), 30))

    def test3_atom(self) -> None:

        target_atom = atom({}, [
            video(VideoLayerParams(
                begin=Second(0), end=Second(10), x=0, y=0, src='/to/path/video.mp4'
            )),
            text(TextLayerParams(
                begin=Second(0), end=Second(20), x=0, y=0, text='Hello, World!'
            ))
        ])

        target_atom(KronReceiver(
            get_children=lambda children: print(children),
            mod_params=lambda params, children: replace(
                params, _offset=Second(0)
            ),
            get_finalized_params=lambda params: print(params),
            get_frame_cnt=lambda frame_cnt: print(frame_cnt),
            get_duration=lambda duration: print(duration),
        ))(KronArgs(Second(5), 30))

    def test4_layer(self) -> None:

        def init():
            return VideoLayerParams(
                begin=Second(0), end=Second(10), x=100, y=0, src='/to/path/video.mp4'
            )

        def update(kronArgs: KronArgs, params: VideoLayerParams):
            return replace(params, x=params.x + 10)

        target_layer = video(init=init(), update=update)

        target_layer(LayerReceiver(
            mod_params=lambda params: replace(params, begin=params.begin + Second(5)),
            get_finalized_params=lambda params: print(params),
            get_frame_cnt=lambda frame_cnt: print(frame_cnt),
            get_duration=lambda duration: print(duration),
        ))(KronArgs(Second(5), 30))

    def test5(self) -> None:

        target_root = root({}, [
            scene({'path': 'path1'}, [
                atom({'duration': Second(15)}, [
                    video(VideoLayerParams(
                        begin=Second(0), end=Second(10), x=0, y=0, src='/to/path/video11.mp4'
                    )),
                    video(VideoLayerParams(
                        begin=Second(0), end=Second(10), x=0, y=0, src='/to/path/video12.mp4'
                    )),
                ]),
                atom({}, [
                    video(VideoLayerParams(
                        begin=Second(0), end=Second(10), x=0, y=0, src='/to/path/video13.mp4'
                    )),
                    text(TextLayerParams(
                        begin=Second(0), end=Second(20), x=0, y=0, text='Hello, World!'
                    )),
                    audio(AudioLayerParams(
                        begin=Second(0), end=Second(20), x=0, y=0, src='/to/path/audio11.mp3'
                    ))
                ]),
            ]),
            scene({'path': 'path2'}, [
                atom({}, [
                    audio(AudioLayerParams(
                        begin=Second(0), end=Second(10), x=0, y=0, src='/to/path/audio21.mp3'
                    ))
                ]),
            ]),

        ])

        print(get_atom_profiles(target_root, KronArgs(Second(1), 30)))


if __name__ == "__main__":
    unittest.main()
