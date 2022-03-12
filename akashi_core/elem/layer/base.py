# pyright: reportPrivateUsage=false
from __future__ import annotations
from dataclasses import dataclass
from abc import ABCMeta
import typing as tp

from akashi_core.elem.context import _GlobalKronContext as gctx
from akashi_core.elem.uuid import UUID, gen_uuid
from akashi_core.time import sec
from akashi_core.pysl.shader import ShaderCompiler

from akashi_core.pysl import _gl

if tp.TYPE_CHECKING:
    from akashi_core.elem.atom import AtomHandle
    from .unit import UnitEntry


LayerKind = tp.Literal['LAYER', 'VIDEO', 'AUDIO', 'TEXT', 'IMAGE', 'UNIT', 'SHAPE', 'FREE']


''' Layer Concept '''

_TLayerTrait = tp.TypeVar('_TLayerTrait', bound='LayerTrait')


@dataclass
class LayerField:
    uuid: UUID = UUID('')
    atom_uuid: UUID = UUID('')
    kind: LayerKind = 'LAYER'
    key: str = ''
    duration: tp.Union[sec, 'AtomHandle'] = sec(5)
    atom_offset: sec = sec(0)


@dataclass
class LayerTrait(metaclass=ABCMeta):

    _idx: int

    def duration(self: '_TLayerTrait', duration: sec | float) -> '_TLayerTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(LayerField, cur_layer).duration = sec(duration)
        return self

    def ap(self: '_TLayerTrait', *hs: tp.Callable[['_TLayerTrait'], '_TLayerTrait']) -> '_TLayerTrait':
        for h in hs:
            h(self)
        return self


''' Position Concept '''


_TPositionTrait = tp.TypeVar('_TPositionTrait', bound='PositionTrait')


@dataclass
class PositionField:
    pos: tuple[int, int] = (0, 0)
    z: float = 0.0
    layer_size: tuple[int, int] = (-1, -1)
    rotation: sec = sec(0)


class PositionTrait(LayerTrait, metaclass=ABCMeta):

    def pos(self: '_TPositionTrait', x: int, y: int) -> '_TPositionTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(PositionField, cur_layer).pos = (x, y)
        return self

    def z(self: '_TPositionTrait', value: float) -> '_TPositionTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(PositionField, cur_layer).z = value
        return self

    def layer_size(self: '_TPositionTrait', width: int, height: int) -> '_TPositionTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(PositionField, cur_layer).layer_size = (width, height)
        return self

    def rotate(self: '_TPositionTrait', degrees: int | float | sec) -> '_TPositionTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(PositionField, cur_layer).rotation = sec(degrees)
        return self


''' Shader Concept '''


frag = _gl._frag
poly = _gl._poly


@dataclass
class ShaderField:
    frag_shader: tp.Optional[ShaderCompiler] = None
    poly_shader: tp.Optional[ShaderCompiler] = None


''' FittableDuration Concept '''

_TFittableDurationTrait = tp.TypeVar('_TFittableDurationTrait', bound='FittableDurationTrait')


class FittableDurationTrait(LayerTrait, metaclass=ABCMeta):
    def fit_to(self: '_TFittableDurationTrait', handle: 'AtomHandle') -> '_TFittableDurationTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(LayerField, cur_layer).duration = handle
        return self


''' Crop Concept '''


_TCropTrait = tp.TypeVar('_TCropTrait', bound='CropTrait')


@dataclass
class CropField:
    crop_begin: tuple[int, int] = (0, 0)
    crop_end: tuple[int, int] = (0, 0)


class CropTrait(LayerTrait, metaclass=ABCMeta):

    def crop_begin(self: '_TCropTrait', x: int, y: int) -> '_TCropTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(CropField, cur_layer).crop_begin = (x, y)
        return self

    def crop_end(self: '_TCropTrait', x: int, y: int) -> '_TCropTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(CropField, cur_layer).crop_end = (x, y)
        return self


''' Texture Concept '''

_TTextureTrait = tp.TypeVar('_TTextureTrait', bound='TextureTrait')


@dataclass
class TextureField:
    flip_v: bool = False
    flip_h: bool = False


class TextureTrait(LayerTrait, metaclass=ABCMeta):

    def flip_v(self: '_TTextureTrait', enable_flag: bool = True) -> '_TTextureTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(TextureField, cur_layer).flip_v = enable_flag
        return self

    def flip_h(self: '_TTextureTrait', enable_flag: bool = True) -> '_TTextureTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(TextureField, cur_layer).flip_h = enable_flag
        return self


def __is_atom_active(atom_uuid: UUID, raise_exp: bool = True) -> bool:

    cur_atom = gctx.get_ctx().atoms[-1]
    r = atom_uuid == cur_atom.uuid
    if raise_exp and not(r):
        raise Exception('Update for an inactive atom is forbidden')
    else:
        return r


def peek_entry(layer_idx: int) -> tp.Optional[LayerField]:

    cur_layer = gctx.get_ctx().layers[layer_idx]

    if not __is_atom_active(cur_layer.atom_uuid, True):
        return None
    else:
        return cur_layer


def register_entry(entry: LayerField, kind: LayerKind, key: str) -> int:

    cur_ctx = gctx.get_ctx()
    cur_atom = cur_ctx.atoms[-1]

    # LayerField
    entry.uuid = gen_uuid()
    entry.atom_uuid = cur_atom.uuid
    entry.kind = kind
    entry.key = key

    cur_ctx.layers.append(entry)
    cur_layer_idx = len(cur_ctx.layers) - 1

    if len(gctx.get_ctx()._cur_unit_ids) == 0:
        cur_atom.layer_indices.append(cur_layer_idx)
    else:
        cur_unit_layer = tp.cast('UnitEntry', cur_ctx.layers[gctx.get_ctx()._cur_unit_ids[-1]])
        cur_unit_layer.layer_indices.append(cur_layer_idx)

    return cur_layer_idx
