# pyright: reportPrivateUsage=false
import unittest
from akashi_core import gl, ak
import typing as tp
import random
from dataclasses import asdict
import json

if tp.TYPE_CHECKING:
    from akashi_core.elem.context import _ElemFnOpaque, KronContext
    from akashi_core.elem.layer import LayerField


def eval_kron(elem_fn: tp.Callable[['_ElemFnOpaque'], 'KronContext'], config_lpath: str, seed: int = 128) -> 'KronContext':
    config_path = tp.cast('_ElemFnOpaque', ak.from_relpath(__file__, config_lpath))
    random.seed(seed)
    return elem_fn(config_path)


def get_layer_kind(layer: 'LayerField') -> str:
    if layer.t_video != -1:
        return 'VIDEO'
    elif layer.t_audio != -1:
        return 'AUDIO'
    elif layer.t_image != -1:
        return 'IMAGE'
    elif layer.t_text != -1:
        return 'TEXT'
    elif layer.t_rect != -1 or layer.t_circle != -1 or layer.t_tri != -1 or layer.t_line != -1:
        return 'SHAPE'
    elif layer.t_unit != -1:
        return 'UNIT'
    else:
        return 'UNKNOWN'


def to_json(obj: tp.Any) -> str:

    class Encoder(json.JSONEncoder):
        def default(self, o: tp.Any) -> tp.Any:
            if hasattr(o, 'to_json'):
                return o.to_json()
            else:
                return json.JSONEncoder.default(self, o)

    return json.dumps(obj, cls=Encoder, ensure_ascii=False, indent=2)


def print_kron(kron: 'KronContext'):
    print(to_json(asdict(kron)))


class TestBasicLayers(unittest.TestCase):

    def test_basic(self):

        @ak.entry()
        def main():
            with ak.rect(200, 200) as t:
                t.t_time.duration(3)

        kron = eval_kron(main, './test_elem_config1.py')

        self.assertEqual(len(kron.atoms), 1)
        self.assertEqual(kron.atoms[0]._duration, ak.sec(3))

        self.assertEqual(len(kron.layers), 1)
        self.assertNotEqual(kron.layers[0].t_rect, -1)
        self.assertEqual(kron.t_rects[kron.layers[0].t_rect].req_size, (200, 200))

        self.assertNotEqual(kron.layers[0].t_transform, -1)
        self.assertEqual(kron.t_transforms[kron.layers[0].t_transform].pos, (960, 540))

    def test_video_basic(self):

        @ak.entry()
        def main():
            vurl = ak.from_relpath(__file__, './resource_fixtures/countdown1/countdown1_720p.mp4')
            with ak.video(vurl):
                ...

        kron = eval_kron(main, './test_elem_config1.py')

        self.assertEqual(len(kron.atoms), 1)
        self.assertEqual(kron.atoms[0]._duration, ak.sec(10027, 1000))

        self.assertEqual(len(kron.layers), 1)
        self.assertNotEqual(kron.layers[0].t_video, -1)
        self.assertEqual(kron.t_videos[kron.layers[0].t_video].start, ak.sec(0))
        self.assertEqual(kron.t_videos[kron.layers[0].t_video].end, ak.sec(10027, 1000))

    def test_video_with_range(self):

        @ak.entry()
        def main():
            vurl = ak.from_relpath(__file__, './resource_fixtures/countdown1/countdown1_720p.mp4')
            with ak.video(vurl) as t:
                t.t_video.range(5, -1)

        kron = eval_kron(main, './test_elem_config1.py')

        self.assertEqual(len(kron.atoms), 1)
        self.assertEqual(kron.atoms[0]._duration, ak.sec(5027, 1000))

        self.assertEqual(len(kron.layers), 1)
        self.assertNotEqual(kron.layers[0].t_video, -1)
        self.assertEqual(kron.t_videos[kron.layers[0].t_video].start, ak.sec(5))
        self.assertEqual(kron.t_videos[kron.layers[0].t_video].end, ak.sec(10027, 1000))

    def test_video_with_span_dur(self):

        @ak.entry()
        def main():
            vurl = ak.from_relpath(__file__, './resource_fixtures/countdown1/countdown1_720p.mp4')
            with ak.video(vurl) as t:
                t.t_video.range(5, -1).span_dur(3)  # short span
            with ak.video(vurl) as t:
                t.t_video.range(5, -1).span_dur(10)  # long span

        kron = eval_kron(main, './test_elem_config1.py')
        # print_kron(kron)

        self.assertEqual(len(kron.atoms), 1)
        self.assertEqual(kron.atoms[0]._duration, ak.sec(10))

        self.assertEqual(len(kron.layers), 2)

        self.assertNotEqual(kron.layers[0].t_video, -1)
        self.assertEqual(kron.t_videos[kron.layers[0].t_video].start, ak.sec(5))
        self.assertEqual(kron.t_videos[kron.layers[0].t_video].end, ak.sec(10027, 1000))
        self.assertEqual(kron.t_videos[kron.layers[0].t_video]._span_dur, ak.sec(3))

        self.assertNotEqual(kron.layers[1].t_video, -1)
        self.assertEqual(kron.t_videos[kron.layers[1].t_video].start, ak.sec(5))
        self.assertEqual(kron.t_videos[kron.layers[1].t_video].end, ak.sec(10027, 1000))
        self.assertEqual(kron.t_videos[kron.layers[1].t_video]._span_dur, ak.sec(10))

    def test_video_with_span_cnt(self):

        @ak.entry()
        def main():
            vurl = ak.from_relpath(__file__, './resource_fixtures/countdown1/countdown1_720p.mp4')
            with ak.video(vurl) as t:
                t.t_video.range(5, 10).span_cnt(10)

        kron = eval_kron(main, './test_elem_config1.py')
        # print_kron(kron)

        self.assertEqual(len(kron.atoms), 1)
        self.assertEqual(kron.atoms[0]._duration, ak.sec(50))

        self.assertEqual(len(kron.layers), 1)
        self.assertNotEqual(kron.layers[0].t_video, -1)
        self.assertEqual(kron.t_videos[kron.layers[0].t_video].start, ak.sec(5))
        self.assertEqual(kron.t_videos[kron.layers[0].t_video].end, ak.sec(10))
        self.assertEqual(kron.t_videos[kron.layers[0].t_video]._span_cnt, ak.sec(10))


class TestFrame(unittest.TestCase):

    def test_basic(self):

        @ak.frame()
        def base_frame(rect_w: int, rect_h: int):
            with ak.rect(rect_w, rect_h) as t:
                t.t_time.duration(3)
            with ak.rect(rect_w, rect_h) as t:
                t.t_time.duration(10)

        @ak.entry()
        def main():
            with ak.unit(base_frame(200, 200)):
                ...

        kron = eval_kron(main, './test_elem_config1.py')

        self.assertEqual(len(kron.atoms), 1)
        self.assertEqual(kron.atoms[0]._duration, ak.sec(10))

        self.assertEqual(len(kron.layers), 3)

        self.assertNotEqual(kron.layers[0].t_unit, -1)
        # [XXX] The duration of Unit Layer should be equal to the max duration of its children
        self.assertEqual(kron.layers[0]._duration, ak.sec(10))
        # [XXX] Unit Layer should have the layer indices of its children
        self.assertEqual(kron.t_units[kron.layers[0].t_unit].layer_indices, [1, 2])
        self.assertEqual(kron.t_units[kron.layers[0].t_unit].fb_size, (1920, 1080))

        self.assertNotEqual(kron.layers[0].t_transform, -1)
        self.assertEqual(kron.t_transforms[kron.layers[0].t_transform].layer_size, (1920, 1080))
        self.assertEqual(kron.t_transforms[kron.layers[0].t_transform].pos, (960, 540))

    def test_nested_units(self):

        @ak.frame()
        def inner_frame():
            with ak.rect(ak.lwidth() // 5, 200) as t:
                t.t_time.duration(3)
                t.t_layer.key('inner')

        @ak.frame()
        def topmost_frame():
            with ak.unit(inner_frame()) as u1:
                u1.t_unit.fb_size(ak.lwidth() // 2, ak.lheight())

            with ak.rect(ak.lwidth() // 5, 200) as t:
                t.t_time.duration(u1.ref())
                t.t_layer.key('topmost')

        @ak.entry()
        def main():
            with ak.unit(topmost_frame()):
                ...

        kron = eval_kron(main, './test_elem_config1.py')
        # print_kron(kron)

        self.assertEqual(len(kron.atoms), 1)
        self.assertEqual(kron.atoms[0]._duration, ak.sec(3))

        self.assertEqual(len(kron.layers), 4)
        self.assertNotEqual(kron.layers[0].t_unit, -1)
        self.assertEqual(kron.layers[0]._duration, ak.sec(3))
        self.assertEqual(kron.t_units[kron.layers[0].t_unit].layer_indices, [1, 3])
        self.assertEqual(kron.t_units[kron.layers[0].t_unit].fb_size, (1920, 1080))
        self.assertNotEqual(kron.layers[0].t_transform, -1)
        self.assertEqual(kron.t_transforms[kron.layers[0].t_transform].layer_size, (1920, 1080))
        self.assertEqual(kron.t_transforms[kron.layers[0].t_transform].pos, (960, 540))

        self.assertNotEqual(kron.layers[1].t_unit, -1)
        self.assertEqual(kron.layers[1]._duration, ak.sec(3))
        self.assertEqual(kron.t_units[kron.layers[1].t_unit].layer_indices, [2])
        self.assertEqual(kron.t_units[kron.layers[1].t_unit].fb_size, (1920 // 2, 1080))
        self.assertNotEqual(kron.layers[1].t_transform, -1)
        self.assertEqual(kron.t_transforms[kron.layers[1].t_transform].layer_size, (1920 // 2, 1080))
        self.assertEqual(kron.t_transforms[kron.layers[1].t_transform].pos, (960, 540))

        self.assertNotEqual(kron.layers[2].t_rect, -1)
        self.assertEqual(kron.layers[2].key, 'inner')
        self.assertEqual(kron.t_rects[kron.layers[2].t_rect].req_size[0], (1920 // 2) // 5)

        self.assertNotEqual(kron.layers[3].t_rect, -1)
        self.assertEqual(kron.layers[3].key, 'topmost')
        self.assertEqual(kron.layers[3]._duration, ak.sec(3))
        self.assertEqual(kron.t_rects[kron.layers[3].t_rect].req_size[0], (1920 // 5))

    def test_nested_layers1(self):

        @ak.entry()
        def main1():
            with ak.text(""):
                # NG
                with ak.text(""):
                    ...

        with self.assertRaisesRegex(Exception, 'Nested layer is forbidden') as _:
            eval_kron(main1, './test_elem_config1.py')

        @ak.frame()
        def topmost_frame():
            with ak.rect(ak.lwidth() // 5, 200) as t:
                t.t_time.duration(3)
                t.t_layer.key('topmost')

                # NG
                with ak.text(""):
                    ...

        @ak.entry()
        def main2():
            with ak.unit(topmost_frame()):
                ...

        with self.assertRaisesRegex(Exception, 'Nested layer is forbidden') as _:
            eval_kron(main2, './test_elem_config1.py')

    def test_timeline_basic(self):

        @ak.frame('timeline')
        def base_frame(rect_w: int, rect_h: int):
            with ak.rect(rect_w, rect_h) as t:
                t.t_time.duration(3)
            with ak.rect(rect_w, rect_h) as t:
                t.t_time.duration(10)

        @ak.entry()
        def main():
            with ak.unit(base_frame(200, 200)):
                ...

        kron = eval_kron(main, './test_elem_config1.py')
        # print_kron(kron)

        self.assertEqual(len(kron.atoms), 1)
        self.assertEqual(kron.atoms[0]._duration, ak.sec(13))

        self.assertEqual(len(kron.layers), 3)

        self.assertNotEqual(kron.layers[0].t_unit, -1)
        self.assertEqual(kron.layers[0]._duration, ak.sec(13))
        self.assertEqual(kron.t_units[kron.layers[0].t_unit].layer_indices, [1, 2])
        self.assertEqual(kron.t_units[kron.layers[0].t_unit].fb_size, (1920, 1080))
        self.assertNotEqual(kron.layers[0].t_transform, -1)
        self.assertEqual(kron.t_transforms[kron.layers[0].t_transform].layer_size, (1920, 1080))
        self.assertEqual(kron.t_transforms[kron.layers[0].t_transform].pos, (960, 540))

    def test_spatial_with_range(self):

        @ak.frame()
        def inner_frame():

            with ak.rect(ak.lwidth() // 5, 200) as t:
                t.t_time.duration(2).offset(1)
                t.t_layer.key('inner_R1')

            vurl = ak.from_relpath(__file__, './resource_fixtures/countdown1/countdown1_720p.mp4')
            with ak.audio(vurl) as t:
                t.t_audio.range(0, 0.7).span_dur(4)
                t.t_layer.key('inner_A1')

        @ak.frame()
        def base_frame():
            with ak.rect(200, 200) as t:
                t.t_time.duration(3).offset(2)
                t.t_layer.key('R1')

            vurl = ak.from_relpath(__file__, './resource_fixtures/countdown1/countdown1_720p.mp4')
            with ak.video(vurl) as t:
                t.t_video.range(5, -1)

            with ak.audio(vurl):
                ...

            with ak.rect(400, 400) as t:
                t.t_time.duration(1).offset(3)
                t.t_layer.key('R2')

            with ak.rect(200, 200) as t:
                t.t_time.duration(1)
                t.t_layer.key('R3')

            with ak.rect(300, 300) as t:
                t.t_time.duration(2).offset(1)
                t.t_layer.key('R4')

            with ak.unit(inner_frame()) as t:
                t.t_unit.range(0, ak.sec(1.5)).span_cnt(2)
                t.t_layer.key('U1')

        @ak.entry()
        def main():
            with ak.unit(base_frame()) as t:
                t.t_unit.range(2, 6)

        kron = eval_kron(main, './test_elem_config1.py')
        test_lines = []
        for layer_idx, layer in enumerate(kron.layers):
            layer_kind = get_layer_kind(layer)
            test_lines.append(
                f'{layer_idx} {layer_kind} {layer.defunct} {layer.key} {layer.slice_offset} {layer._duration} {layer.layer_local_offset}')

        expected_lines = [e.strip() for e in '''
        0 UNIT False  0 4 0
        1 SHAPE False R1 0 3 0
        2 VIDEO False  0 3027/1000 2
        3 AUDIO False  0 4 2
        4 SHAPE False R2 1 1 0
        5 SHAPE True R3 0 1 0
        6 SHAPE False R4 0 1 1
        7 UNIT False U1 0 1 2
        8 SHAPE True inner_R1 1 1/2 0
        9 AUDIO True inner_A1 0 3/2 0
        10 SHAPE False inner_R1__span_cnt_1 1/2 1/2 0
        11 AUDIO False inner_A1__span_cnt_1 0 1 1/2
        '''.split('\n') if len(e.strip()) > 0]

        self.assertEqual(len(kron.atoms), 1)
        self.assertEqual(kron.atoms[0]._duration, ak.sec(4))
        self.assertEqual(len(kron.layers), 12)

        # layer 5 is defunct layer
        self.assertNotEqual(kron.layers[0].t_unit, -1)
        self.assertEqual(kron.t_units[kron.layers[0].t_unit].layer_indices, [1, 2, 3, 4, 6, 7])
        # layer 8, 9 are defunct layers
        self.assertNotEqual(kron.layers[7].t_unit, -1)
        self.assertEqual(kron.t_units[kron.layers[7].t_unit].layer_indices, [10, 11])

    def test_spatial_with_span_cnt(self):

        @ak.frame()
        def inner_frame():
            with ak.rect(ak.lwidth() // 5, 200) as t:
                t.t_time.duration(2).offset(1)
                t.t_layer.key('inner_R1')

            vurl = ak.from_relpath(__file__, './resource_fixtures/countdown1/countdown1_720p.mp4')
            with ak.audio(vurl) as t:
                t.t_audio.range(0, 0.7).span_dur(4)
                t.t_layer.key('inner_A1')

        @ak.frame()
        def base_frame():

            with ak.rect(200, 200) as t:
                t.t_time.duration(3).offset(2)
                t.t_layer.key('R1')

            vurl = ak.from_relpath(__file__, './resource_fixtures/countdown1/countdown1_720p.mp4')
            with ak.video(vurl) as t:
                t.t_video.range(5, -1)
                t.t_layer.key('V1')

            with ak.audio(vurl) as t:
                t.t_audio.range(0, 1.5).span_dur(6)
                t.t_layer.key('A1')

            with ak.rect(400, 400) as t:
                t.t_time.duration(1).offset(3)
                t.t_layer.key('R2')

            with ak.rect(200, 200) as t:
                t.t_time.duration(1)
                t.t_layer.key('R3')

            with ak.rect(300, 300) as t:
                t.t_time.duration(2).offset(1)
                t.t_layer.key('R4')

            with ak.unit(inner_frame()) as t:
                t.t_unit.range(0, ak.sec(1.5)).span_cnt(2)
                t.t_layer.key('U1')

        @ak.entry()
        def main():
            with ak.unit(base_frame()) as t:
                t.t_unit.range(2, 6).span_cnt(2)
                t.t_layer.key('MainUnit')

        kron = eval_kron(main, './test_elem_config1.py')
        test_lines = []
        for layer_idx, layer in enumerate(kron.layers):
            layer_kind = get_layer_kind(layer)
            test_lines.append(
                f'{layer_idx} {layer_kind} {layer.defunct} {layer.key} {layer.slice_offset} {layer._duration} {layer.layer_local_offset}')

        # print('\n'.join(test_lines))

        expected_lines = [e.strip() for e in '''
        0 UNIT False MainUnit 0 8 0
        1 SHAPE False R1 0 3 0
        2 VIDEO False V1 0 3027/1000 2
        3 AUDIO False A1 0 4 1/2
        4 SHAPE False R2 1 1 0
        5 SHAPE True R3 0 1 0
        6 SHAPE False R4 0 1 1
        7 UNIT False U1 0 1 2
        8 SHAPE True inner_R1 1 1/2 0
        9 AUDIO True inner_A1 0 3/2 0
        10 SHAPE False inner_R1__span_cnt_1 1/2 1/2 0
        11 AUDIO False inner_A1__span_cnt_1 0 1 1/2
        12 SHAPE False R1__span_cnt_1 4 3 0
        13 VIDEO False V1__span_cnt_1 4 3027/1000 2
        14 AUDIO False A1__span_cnt_1 4 4 1/2
        15 SHAPE False R2__span_cnt_1 5 1 0
        16 SHAPE False R4__span_cnt_1 4 1 1
        17 UNIT False U1__span_cnt_1 4 1 2
        18 SHAPE False inner_R1__span_cnt_1__span_cnt_1 9/2 1/2 0
        19 AUDIO False inner_A1__span_cnt_1__span_cnt_1 4 1 1/2
        '''.split('\n') if len(e.strip()) > 0]

        self.assertEqual(kron.atoms[0]._duration, ak.sec(8))
        self.assertEqual(len(kron.layers), 20)

        # Every unit layers should have only its immediate children as reference elements.

        # layer 5 is defunct layers
        self.assertNotEqual(kron.layers[0].t_unit, -1)
        self.assertEqual(kron.t_units[kron.layers[0].t_unit].layer_indices, [
            1, 2, 3, 4, 6, 7, 12, 13, 14, 15, 16, 17])
        # layer 8, 9 are defunct layers
        self.assertNotEqual(kron.layers[7].t_unit, -1)
        self.assertEqual(kron.t_units[kron.layers[7].t_unit].layer_indices, [
            10, 11])
        self.assertNotEqual(kron.layers[17].t_unit, -1)
        self.assertEqual(kron.t_units[kron.layers[17].t_unit].layer_indices, [
            18, 19])

        self.assertEqual(test_lines, expected_lines)

    def test_spatial_with_span_cnt_and_timeline(self):

        @ak.frame()
        def inner2_frame():
            with ak.rect(ak.lwidth() // 5, 200) as t:
                t.t_time.duration(10)
                t.t_layer.key('inner2_R1')

        @ak.frame('timeline')
        def inner_frame(rect_w: int, rect_h: int):
            with ak.rect(rect_w, rect_h) as t:
                t.t_time.duration(3)
                t.t_layer.key('timeline_R1')

            with ak.unit(inner2_frame()) as t:
                t.t_layer.key('timeline_U1')

        @ak.frame()
        def base_frame():

            with ak.unit(inner_frame(500, 500)) as t:
                t.t_layer.key('U1')

            with ak.rect(ak.lwidth() // 5, 200) as t:
                t.t_time.duration(2).offset(1)
                t.t_layer.key('R1')

            vurl = ak.from_relpath(__file__, './resource_fixtures/countdown1/countdown1_720p.mp4')
            with ak.audio(vurl) as t:
                t.t_audio.range(0, 0.7).span_dur(4)
                t.t_layer.key('A1')

        @ak.entry()
        def main():
            with ak.unit(base_frame()) as t:
                t.t_unit.range(2, 5).span_cnt(2)
                t.t_layer.key('MainUnit')

        kron = eval_kron(main, './test_elem_config1.py')
        test_lines = []
        for layer_idx, layer in enumerate(kron.layers):
            layer_kind = get_layer_kind(layer)
            test_lines.append(
                f'{layer_idx} {layer_kind} {layer.defunct} {layer.key} {layer.slice_offset} {layer._duration} {layer.layer_local_offset}')

        # print('\n'.join(test_lines))

        expected_lines = [e.strip() for e in '''
        0 UNIT False MainUnit 0 6 0
        1 UNIT False U1 0 3 2
        2 SHAPE False timeline_R1 0 1 2
        3 UNIT False timeline_U1 1 2 0
        4 SHAPE False inner2_R1 1 2 0
        5 SHAPE False R1 0 1 1
        6 AUDIO False A1 0 2 3/5
        7 UNIT False U1__span_cnt_1 3 3 2
        8 SHAPE False timeline_R1__span_cnt_1 3 1 2
        9 UNIT False timeline_U1__span_cnt_1 4 2 0
        10 SHAPE False inner2_R1__span_cnt_1 4 2 0
        11 SHAPE False R1__span_cnt_1 3 1 1
        12 AUDIO False A1__span_cnt_1 3 2 3/5
        '''.split('\n') if len(e.strip()) > 0]

        self.assertEqual(test_lines, expected_lines)

    def test_timeline_with_span_cnt(self):

        @ak.frame('timeline')
        def base_frame(rect_w: int, rect_h: int):
            with ak.rect(rect_w, rect_h) as t:
                t.t_layer.key('timeline_R1')
                t.t_time.duration(3)
            with ak.rect(rect_w, rect_h) as t:
                t.t_layer.key('timeline_R2')
                t.t_time.duration(10)

        @ak.entry()
        def main():
            with ak.unit(base_frame(200, 200)) as t:
                t.t_layer.key('MainUnit')
                t.t_unit.range(2, 5).span_cnt(2)

        kron = eval_kron(main, './test_elem_config1.py')
        test_lines = []
        for layer_idx, layer in enumerate(kron.layers):
            layer_kind = get_layer_kind(layer)
            test_lines.append(
                f'{layer_idx} {layer_kind} {layer.defunct} {layer.key} {layer.slice_offset} {layer._duration} {layer.layer_local_offset}')

        # print('\n'.join(test_lines))

        expected_lines = [e.strip() for e in '''
        0 UNIT False MainUnit 0 6 0
        1 SHAPE False timeline_R1 0 1 2
        2 SHAPE False timeline_R2 1 2 0
        3 SHAPE False timeline_R1__span_cnt_1 3 1 2
        4 SHAPE False timeline_R2__span_cnt_1 4 2 0
        '''.split('\n') if len(e.strip()) > 0]

        self.assertEqual(test_lines, expected_lines)
        self.assertNotEqual(kron.layers[0].t_unit, -1)
        self.assertEqual(kron.t_units[kron.layers[0].t_unit].layer_indices, [1, 2, 3, 4])
