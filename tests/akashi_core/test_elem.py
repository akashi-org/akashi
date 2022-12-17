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
                t.key('topmost')
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

        @ak.entry()
        def main():
            ak.unit(base_frame(), lambda t: (
                t.range(2, 6)
            ))

        kron = eval_kron(main, './test_elem_config1.py')
        # print_kron(kron)

        self.assertEqual(len(kron.atoms), 1)
        self.assertEqual(kron.atoms[0]._duration, ak.sec(4))

        self.assertEqual(len(kron.layers), 7)
        self.assertEqual(kron.layers[0].kind, 'UNIT')
        self.assertEqual(kron.layers[0]._duration, ak.sec(4))
        self.assertEqual(tp.cast(HasUnitLocalField, kron.layers[0]).unit.layer_indices, [1, 2, 3, 4, 6])

        self.assertEqual(kron.layers[1].kind, 'SHAPE')
        self.assertEqual(kron.layers[1].key, 'R1')
        self.assertEqual(kron.layers[1].slice_offset, ak.sec(0))
        self.assertEqual(kron.layers[1]._duration, ak.sec(3))
        self.assertEqual(kron.layers[1].layer_local_offset, ak.sec(0))

        self.assertEqual(kron.layers[2].kind, 'VIDEO')
        self.assertEqual(kron.layers[2].slice_offset, ak.sec(0))
        self.assertEqual(kron.layers[2]._duration, ak.sec(3027, 1000))
        self.assertEqual(kron.layers[2].layer_local_offset, ak.sec(2))
        self.assertEqual(tp.cast(HasMediaField, kron.layers[2]).media.start, ak.sec(7))
        self.assertEqual(tp.cast(HasMediaField, kron.layers[2]).media.end, ak.sec(10027, 1000))

        self.assertEqual(kron.layers[3].kind, 'AUDIO')
        self.assertEqual(kron.layers[3].slice_offset, ak.sec(0))
        self.assertEqual(kron.layers[3]._duration, ak.sec(4))
        self.assertEqual(kron.layers[3].layer_local_offset, ak.sec(2))
        self.assertEqual(tp.cast(HasMediaField, kron.layers[3]).media.start, ak.sec(2))
        self.assertEqual(tp.cast(HasMediaField, kron.layers[3]).media.end, ak.sec(6))

        self.assertEqual(kron.layers[4].kind, 'SHAPE')
        self.assertEqual(kron.layers[4].key, 'R2')
        self.assertEqual(kron.layers[4].slice_offset, ak.sec(1))
        self.assertEqual(kron.layers[4]._duration, ak.sec(1))
        self.assertEqual(kron.layers[4].layer_local_offset, ak.sec(0))

        self.assertEqual(kron.layers[5].kind, 'SHAPE')
        self.assertEqual(kron.layers[5].key, 'R3')
        self.assertEqual(kron.layers[5].defunct, True)

        self.assertEqual(kron.layers[6].kind, 'SHAPE')
        self.assertEqual(kron.layers[6].key, 'R4')
        self.assertEqual(kron.layers[6].slice_offset, ak.sec(0))
        self.assertEqual(kron.layers[6]._duration, ak.sec(1))
        self.assertEqual(kron.layers[6].layer_local_offset, ak.sec(1))
