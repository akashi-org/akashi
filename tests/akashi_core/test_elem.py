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


class TestUnit(unittest.TestCase):

    def test_basic(self):

        @ak.entry()
        def main():
            with ak.unit():
                ak.rect(200, 200, lambda t: (
                    t.duration(3)
                ))
                ak.rect(200, 200, lambda t: (
                    t.duration(10)
                ))

        kron = eval_kron(main, './test_elem_config1.py')
        # print_kron(kron)

        self.assertEqual(len(kron.atoms), 1)
        self.assertEqual(kron.atoms[0]._duration, ak.sec(10))

        self.assertEqual(len(kron.layers), 3)
        self.assertEqual(kron.layers[0].kind, 'UNIT')
        # [XXX] The duration of Unit Layer should be equal to the max duration of its children
        self.assertEqual(kron.layers[0].duration, ak.sec(10))
        # [XXX] Unit Layer should have the layer indices of its children
        self.assertEqual(tp.cast(HasUnitLocalField, kron.layers[0]).unit.layer_indices, [1, 2])
        self.assertEqual(tp.cast(HasUnitLocalField, kron.layers[0]).unit.fb_size, (1920, 1080))
        self.assertEqual(tp.cast(HasTransformField, kron.layers[0]).transform.layer_size, (1920, 1080))
        self.assertEqual(tp.cast(HasTransformField, kron.layers[0]).transform.pos, (960, 540))

        self.assertEqual(kron.layers[1].kind, 'SHAPE')
        self.assertEqual(kron.layers[2].kind, 'SHAPE')

    def test_nested_units(self):

        @ak.entry()
        def main():
            with ak.unit():
                with ak.unit() as u:
                    ak.rect(200, 200, lambda t: (
                        t.duration(3)
                    ))
                ak.rect(200, 200, lambda t: (
                    t.duration(u)
                ))

        kron = eval_kron(main, './test_elem_config1.py')
        # print_kron(kron)

        self.assertEqual(len(kron.atoms), 1)
        self.assertEqual(kron.atoms[0]._duration, ak.sec(3))

        self.assertEqual(len(kron.layers), 4)
        self.assertEqual(kron.layers[0].kind, 'UNIT')
        self.assertEqual(kron.layers[0].duration, ak.sec(3))
        self.assertEqual(tp.cast(HasUnitLocalField, kron.layers[0]).unit.layer_indices, [1, 3])
        self.assertEqual(tp.cast(HasUnitLocalField, kron.layers[0]).unit.fb_size, (1920, 1080))
        self.assertEqual(tp.cast(HasTransformField, kron.layers[0]).transform.layer_size, (1920, 1080))
        self.assertEqual(tp.cast(HasTransformField, kron.layers[0]).transform.pos, (960, 540))

        self.assertEqual(kron.layers[1].kind, 'UNIT')
        self.assertEqual(kron.layers[1].duration, ak.sec(3))
        self.assertEqual(tp.cast(HasUnitLocalField, kron.layers[1]).unit.layer_indices, [2])
        self.assertEqual(tp.cast(HasUnitLocalField, kron.layers[1]).unit.fb_size, (1920, 1080))
        self.assertEqual(tp.cast(HasTransformField, kron.layers[1]).transform.layer_size, (1920, 1080))
        self.assertEqual(tp.cast(HasTransformField, kron.layers[1]).transform.pos, (960, 540))

        self.assertEqual(kron.layers[2].kind, 'SHAPE')

        self.assertEqual(kron.layers[3].kind, 'SHAPE')
        self.assertEqual(kron.layers[3].duration, ak.sec(3))
