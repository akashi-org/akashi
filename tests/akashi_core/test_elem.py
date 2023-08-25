# pyright: reportPrivateUsage=false
import unittest
from akashi_core import gl, ak
from akashi_core.elem.layer.base import HasTransformField, HasMediaField
from akashi_core.elem.layer.shape import HasShapeLocalField
from akashi_core.elem.layer.unit import HasUnitLocalField
import typing as tp
import random
from dataclasses import asdict
import json

if tp.TYPE_CHECKING:
    from akashi_core.elem.context import _ElemFnOpaque, KronContext


def eval_kron(elem_fn: tp.Callable[['_ElemFnOpaque'], 'KronContext'], config_lpath: str, seed: int = 128) -> 'KronContext':
    config_path = tp.cast('_ElemFnOpaque', ak.from_relpath(__file__, config_lpath))
    random.seed(seed)
    return elem_fn(config_path)


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
            ak.rect(200, 200, lambda t: (
                t.duration(3)
            ))

        kron = eval_kron(main, './test_elem_config1.py')

        self.assertEqual(len(kron.atoms), 1)
        self.assertEqual(kron.atoms[0]._duration, ak.sec(3))

        self.assertEqual(len(kron.layers), 1)
        self.assertEqual(kron.layers[0].kind, 'SHAPE')
        self.assertEqual(tp.cast(HasShapeLocalField, kron.layers[0]).shape.shape_kind, 'RECT')
        self.assertEqual(tp.cast(HasTransformField, kron.layers[0]).transform.pos, (960, 540))

    def test_video_basic(self):

        @ak.entry()
        def main():
            vurl = ak.from_relpath(__file__, './resource_fixtures/countdown1/countdown1_720p.mp4')
            ak.video(vurl, lambda t: ())

        kron = eval_kron(main, './test_elem_config1.py')

        self.assertEqual(len(kron.atoms), 1)
        self.assertEqual(kron.atoms[0]._duration, ak.sec(10027, 1000))

        self.assertEqual(len(kron.layers), 1)
        self.assertEqual(kron.layers[0].kind, 'VIDEO')
        self.assertEqual(tp.cast(HasMediaField, kron.layers[0]).media.start, ak.sec(0))
        self.assertEqual(tp.cast(HasMediaField, kron.layers[0]).media.end, ak.sec(10027, 1000))

    def test_video_with_range(self):

        @ak.entry()
        def main():
            vurl = ak.from_relpath(__file__, './resource_fixtures/countdown1/countdown1_720p.mp4')
            ak.video(vurl, lambda t: (
                t.media.range(5, -1)
            ))

        kron = eval_kron(main, './test_elem_config1.py')

        self.assertEqual(len(kron.atoms), 1)
        self.assertEqual(kron.atoms[0]._duration, ak.sec(5027, 1000))

        self.assertEqual(len(kron.layers), 1)
        self.assertEqual(kron.layers[0].kind, 'VIDEO')
        self.assertEqual(tp.cast(HasMediaField, kron.layers[0]).media.start, ak.sec(5))
        self.assertEqual(tp.cast(HasMediaField, kron.layers[0]).media.end, ak.sec(10027, 1000))

    def test_video_with_span_dur(self):

        @ak.entry()
        def main():
            vurl = ak.from_relpath(__file__, './resource_fixtures/countdown1/countdown1_720p.mp4')
            ak.video(vurl, lambda t: (
                t.media.range(5, -1).span_dur(3)  # short span
            ))
            ak.video(vurl, lambda t: (
                t.media.range(5, -1).span_dur(10)  # long span
            ))

        kron = eval_kron(main, './test_elem_config1.py')
        # print_kron(kron)

        self.assertEqual(len(kron.atoms), 1)
        self.assertEqual(kron.atoms[0]._duration, ak.sec(10))

        self.assertEqual(len(kron.layers), 2)
        self.assertEqual(kron.layers[0].kind, 'VIDEO')
        self.assertEqual(tp.cast(HasMediaField, kron.layers[0]).media.start, ak.sec(5))
        self.assertEqual(tp.cast(HasMediaField, kron.layers[0]).media.end, ak.sec(10027, 1000))  # !!!
        self.assertEqual(tp.cast(HasMediaField, kron.layers[0]).media._span_dur, ak.sec(3))

        self.assertEqual(kron.layers[1].kind, 'VIDEO')
        self.assertEqual(tp.cast(HasMediaField, kron.layers[1]).media.start, ak.sec(5))
        self.assertEqual(tp.cast(HasMediaField, kron.layers[1]).media.end, ak.sec(10027, 1000))  # !!!
        self.assertEqual(tp.cast(HasMediaField, kron.layers[1]).media._span_dur, ak.sec(10))

    def test_video_with_span_cnt(self):

        @ak.entry()
        def main():
            vurl = ak.from_relpath(__file__, './resource_fixtures/countdown1/countdown1_720p.mp4')
            ak.video(vurl, lambda t: (
                t.media.range(5, 10).span_cnt(10)
            ))

        kron = eval_kron(main, './test_elem_config1.py')
        # print_kron(kron)

        self.assertEqual(len(kron.atoms), 1)
        self.assertEqual(kron.atoms[0]._duration, ak.sec(50))

        self.assertEqual(len(kron.layers), 1)
        self.assertEqual(kron.layers[0].kind, 'VIDEO')
        self.assertEqual(tp.cast(HasMediaField, kron.layers[0]).media.start, ak.sec(5))
        self.assertEqual(tp.cast(HasMediaField, kron.layers[0]).media.end, ak.sec(10))
        self.assertEqual(tp.cast(HasMediaField, kron.layers[0]).media._span_cnt, 10)


class TestFrame(unittest.TestCase):

    def test_basic(self):

        @ak.frame()
        def base_frame(rect_w: int, rect_h: int):
            ak.rect(rect_w, rect_h, lambda t: (
                t.duration(3)
            ))
            ak.rect(rect_w, rect_h, lambda t: (
                t.duration(10)
            ))

        @ak.entry()
        def main():
            ak.unit(base_frame(200, 200))

        kron = eval_kron(main, './test_elem_config1.py')
        # print_kron(kron)

        self.assertEqual(len(kron.atoms), 1)
        self.assertEqual(kron.atoms[0]._duration, ak.sec(10))

        self.assertEqual(len(kron.layers), 3)
        self.assertEqual(kron.layers[0].kind, 'UNIT')
        # [XXX] The duration of Unit Layer should be equal to the max duration of its children
        self.assertEqual(kron.layers[0]._duration, ak.sec(10))
        # [XXX] Unit Layer should have the layer indices of its children
        self.assertEqual(tp.cast(HasUnitLocalField, kron.layers[0]).unit.layer_indices, [1, 2])
        self.assertEqual(tp.cast(HasUnitLocalField, kron.layers[0]).unit.fb_size, (1920, 1080))
        self.assertEqual(tp.cast(HasTransformField, kron.layers[0]).transform.layer_size, (1920, 1080))
        self.assertEqual(tp.cast(HasTransformField, kron.layers[0]).transform.pos, (960, 540))

        self.assertEqual(kron.layers[1].kind, 'SHAPE')
        self.assertEqual(kron.layers[2].kind, 'SHAPE')

    def test_nested_units(self):

        @ak.frame()
        def inner_frame():
            ak.rect(ak.lwidth() // 5, 200, lambda t: (
                t.duration(3),
                t.key('inner')
            ))

        @ak.frame()
        def topmost_frame():
            l = ak.unit(inner_frame(), lambda t: (
                t.fb_size(ak.lwidth() // 2, ak.lheight()),
            ))
            ak.rect(ak.lwidth() // 5, 200, lambda t: (
                t.duration(l),
                t.key('topmost'),
            ))

        @ak.entry()
        def main():
            ak.unit(topmost_frame())

        kron = eval_kron(main, './test_elem_config1.py')
        # print_kron(kron)

        self.assertEqual(len(kron.atoms), 1)
        self.assertEqual(kron.atoms[0]._duration, ak.sec(3))

        self.assertEqual(len(kron.layers), 4)
        self.assertEqual(kron.layers[0].kind, 'UNIT')
        self.assertEqual(kron.layers[0]._duration, ak.sec(3))
        self.assertEqual(tp.cast(HasUnitLocalField, kron.layers[0]).unit.layer_indices, [1, 3])
        self.assertEqual(tp.cast(HasUnitLocalField, kron.layers[0]).unit.fb_size, (1920, 1080))
        self.assertEqual(tp.cast(HasTransformField, kron.layers[0]).transform.layer_size, (1920, 1080))
        self.assertEqual(tp.cast(HasTransformField, kron.layers[0]).transform.pos, (960, 540))

        self.assertEqual(kron.layers[1].kind, 'UNIT')
        self.assertEqual(kron.layers[1]._duration, ak.sec(3))
        self.assertEqual(tp.cast(HasUnitLocalField, kron.layers[1]).unit.layer_indices, [2])
        self.assertEqual(tp.cast(HasUnitLocalField, kron.layers[1]).unit.fb_size, (1920 // 2, 1080))
        self.assertEqual(tp.cast(HasTransformField, kron.layers[1]).transform.layer_size, (1920 // 2, 1080))
        self.assertEqual(tp.cast(HasTransformField, kron.layers[1]).transform.pos, (960, 540))

        self.assertEqual(kron.layers[2].kind, 'SHAPE')
        self.assertEqual(kron.layers[2].key, 'inner')
        self.assertEqual(tp.cast(HasShapeLocalField, kron.layers[2]).shape.rect.width, (1920 // 2) // 5)

        self.assertEqual(kron.layers[3].kind, 'SHAPE')
        self.assertEqual(kron.layers[3].key, 'topmost')
        self.assertEqual(kron.layers[3]._duration, ak.sec(3))
        self.assertEqual(tp.cast(HasShapeLocalField, kron.layers[3]).shape.rect.width, (1920 // 5))

    def test_timeline_basic(self):

        @ak.frame('timeline')
        def base_frame(rect_w: int, rect_h: int):
            ak.rect(rect_w, rect_h, lambda t: (
                t.duration(3)
            ))
            ak.rect(rect_w, rect_h, lambda t: (
                t.duration(10)
            ))

        @ak.entry()
        def main():
            ak.unit(base_frame(200, 200))

        kron = eval_kron(main, './test_elem_config1.py')
        # print_kron(kron)

        self.assertEqual(len(kron.atoms), 1)
        self.assertEqual(kron.atoms[0]._duration, ak.sec(13))

        self.assertEqual(len(kron.layers), 3)
        self.assertEqual(kron.layers[0].kind, 'UNIT')
        self.assertEqual(kron.layers[0]._duration, ak.sec(13))
        self.assertEqual(tp.cast(HasUnitLocalField, kron.layers[0]).unit.layer_indices, [1, 2])
        self.assertEqual(tp.cast(HasUnitLocalField, kron.layers[0]).unit.fb_size, (1920, 1080))
        self.assertEqual(tp.cast(HasTransformField, kron.layers[0]).transform.layer_size, (1920, 1080))
        self.assertEqual(tp.cast(HasTransformField, kron.layers[0]).transform.pos, (960, 540))

        self.assertEqual(kron.layers[1].kind, 'SHAPE')
        self.assertEqual(kron.layers[2].kind, 'SHAPE')

    def test_spatial_with_range(self):

        @ak.frame()
        def inner_frame():
            ak.rect(ak.lwidth() // 5, 200, lambda t: (
                t.duration(2).offset(1),
                t.key('inner_R1')
            ))

            vurl = ak.from_relpath(__file__, './resource_fixtures/countdown1/countdown1_720p.mp4')
            ak.audio(vurl, lambda t: (
                t.media.range(0, 0.7).span_dur(4),
                t.key('inner_A1')
            ))

        @ak.frame()
        def base_frame():
            ak.rect(200, 200, lambda t: (
                t.duration(3).offset(2),
                t.key('R1')
            ))
            vurl = ak.from_relpath(__file__, './resource_fixtures/countdown1/countdown1_720p.mp4')
            ak.video(vurl, lambda t: (
                t.media.range(5, -1)
            ))
            ak.audio(vurl)
            ak.rect(400, 400, lambda t: (
                t.duration(1).offset(3),
                t.key('R2')
            ))
            ak.rect(200, 200, lambda t: (
                t.duration(1),
                t.key('R3')
            ))
            ak.rect(300, 300, lambda t: (
                t.duration(2).offset(1),
                t.key('R4')
            ))
            ak.unit(inner_frame(), lambda t: (
                t.range(0, ak.sec(1.5)).span_cnt(2),
                t.key('U1')
            ))

        @ak.entry()
        def main():
            ak.unit(base_frame(), lambda t: (
                t.range(2, 6)
            ))

        kron = eval_kron(main, './test_elem_config1.py')
        test_lines = []
        for layer_idx, layer in enumerate(kron.layers):
            test_lines.append(
                f'{layer_idx} {layer.kind} {layer.defunct} {layer.key} {layer.slice_offset} {layer._duration} {layer.layer_local_offset}')

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
        self.assertEqual(tp.cast(HasUnitLocalField, kron.layers[0]).unit.layer_indices, [1, 2, 3, 4, 6, 7])
        # layer 8, 9 are defunct layers
        self.assertEqual(tp.cast(HasUnitLocalField, kron.layers[7]).unit.layer_indices, [10, 11])

    def test_spatial_with_span_cnt(self):

        @ak.frame()
        def inner_frame():
            ak.rect(ak.lwidth() // 5, 200, lambda t: (
                t.duration(2).offset(1),
                t.key('inner_R1')
            ))

            vurl = ak.from_relpath(__file__, './resource_fixtures/countdown1/countdown1_720p.mp4')
            ak.audio(vurl, lambda t: (
                t.media.range(0, 0.7).span_dur(4),
                t.key('inner_A1')
            ))

        @ak.frame()
        def base_frame():
            ak.rect(200, 200, lambda t: (
                t.duration(3).offset(2),
                t.key('R1')
            ))
            vurl = ak.from_relpath(__file__, './resource_fixtures/countdown1/countdown1_720p.mp4')
            ak.video(vurl, lambda t: (
                t.media.range(5, -1),
                t.key('V1')
            ))
            ak.audio(vurl, lambda t: (
                t.media.range(0, 1.5).span_dur(6),
                t.key('A1')
            ))
            ak.rect(400, 400, lambda t: (
                t.duration(1).offset(3),
                t.key('R2')
            ))
            ak.rect(200, 200, lambda t: (
                t.duration(1),
                t.key('R3')
            ))
            ak.rect(300, 300, lambda t: (
                t.duration(2).offset(1),
                t.key('R4')
            ))
            ak.unit(inner_frame(), lambda t: (
                t.range(0, ak.sec(1.5)).span_cnt(2),
                t.key('U1')
            ))

        @ak.entry()
        def main():
            ak.unit(base_frame(), lambda t: (
                t.range(2, 6).span_cnt(2),
                t.key('MainUnit')
            ))

        kron = eval_kron(main, './test_elem_config1.py')
        test_lines = []
        for layer_idx, layer in enumerate(kron.layers):
            test_lines.append(
                f'{layer_idx} {layer.kind} {layer.defunct} {layer.key} {layer.slice_offset} {layer._duration} {layer.layer_local_offset}')

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
        self.assertEqual(tp.cast(HasUnitLocalField, kron.layers[0]).unit.layer_indices, [
            1, 2, 3, 4, 6, 7, 12, 13, 14, 15, 16, 17])
        # layer 8, 9 are defunct layers
        self.assertEqual(tp.cast(HasUnitLocalField, kron.layers[7]).unit.layer_indices, [
            10, 11])
        self.assertEqual(tp.cast(HasUnitLocalField, kron.layers[17]).unit.layer_indices, [
            18, 19])

        self.assertEqual(test_lines, expected_lines)
