# pyright: reportPrivateUsage=false
from __future__ import annotations
from dataclasses import dataclass
import typing as tp
from typing import runtime_checkable

from akashi_core.elem.context import _GlobalKronContext as gctx
from akashi_core.elem.uuid import UUID, gen_uuid
from akashi_core.time import sec, NOT_FIXED_SEC
from akashi_core.pysl.shader import ShaderCompiler

from akashi_core.pysl import _gl

if tp.TYPE_CHECKING:
    from akashi_core.elem.atom import AtomHandle
    from .unit import UnitEntry, UnitHandle


LayerKind = tp.Literal['LAYER', 'VIDEO', 'AUDIO', 'TEXT', 'IMAGE', 'UNIT', 'SHAPE', 'FREE']


''' Layer Concept '''

_TLayerTrait = tp.TypeVar('_TLayerTrait', bound='LayerTrait')


@dataclass
class LayerField:
    uuid: UUID = UUID('')
    atom_uuid: UUID = UUID('')
    kind: LayerKind = 'LAYER'
    key: str = ''
    duration: sec = NOT_FIXED_SEC
    _duration: sec | 'AtomHandle' | 'UnitHandle' = sec(5)
    atom_offset: sec = sec(0)


@dataclass
class LayerTrait:

    _idx: int

    def duration(self: '_TLayerTrait', duration: sec | float | 'UnitHandle' | 'AtomHandle') -> '_TLayerTrait':
        if (cur_layer := peek_entry(self._idx)):
            _duration = sec(duration) if isinstance(duration, (int, float)) else duration
            tp.cast(LayerField, cur_layer)._duration = _duration
        return self

    def offset(self: '_TLayerTrait', offset: sec | float) -> '_TLayerTrait':
        if (cur_layer := peek_entry(self._idx)):
            cur_layer.atom_offset = sec(offset)
        return self

    def ap(self: '_TLayerTrait', h: tp.Callable[['_TLayerTrait'], tp.Any]) -> '_TLayerTrait':
        h(self)
        return self


''' Transform Concept '''


@dataclass
class TransformField:
    pos: tuple[int, int] = (0, 0)
    z: float = 0.0
    layer_size: tuple[int, int] = (-1, -1)
    rotation: sec = sec(0)


@runtime_checkable
class HasTransformField(tp.Protocol):
    transform: TransformField


@dataclass
class TransformTrait:

    _idx: int

    def pos(self, x: int, y: int) -> 'TransformTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(HasTransformField, cur_layer).transform.pos = (x, y)
        return self

    def z(self, value: float) -> 'TransformTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(HasTransformField, cur_layer).transform.z = value
        return self

    def layer_size(self, width: int, height: int) -> 'TransformTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(HasTransformField, cur_layer).transform.layer_size = (width, height)
        return self

    def rotate(self, degrees: int | float | sec) -> 'TransformTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(HasTransformField, cur_layer).transform.rotation = sec(degrees)
        return self


''' Shader Concept '''


frag = _gl._frag
poly = _gl._poly


@dataclass
class ShaderField:
    frag_shader: tp.Optional[ShaderCompiler] = None
    poly_shader: tp.Optional[ShaderCompiler] = None


''' Crop Concept '''


@dataclass
class CropField:
    crop_begin: tuple[int, int] = (0, 0)
    crop_end: tuple[int, int] = (0, 0)


@runtime_checkable
class HasCropField(tp.Protocol):
    crop: CropField


@dataclass
class CropTrait:

    _idx: int

    def crop_begin(self, x: int, y: int) -> 'CropTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(HasCropField, cur_layer).crop.crop_begin = (x, y)
        return self

    def crop_end(self, x: int, y: int) -> 'CropTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(HasCropField, cur_layer).crop.crop_end = (x, y)
        return self


''' Texture Concept '''


@dataclass
class TextureField:
    flip_v: bool = False
    flip_h: bool = False


@runtime_checkable
class HasTextureField(tp.Protocol):
    tex: TextureField


@dataclass
class TextureTrait:

    _idx: int

    def flip_v(self, enable_flag: bool = True) -> 'TextureTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(HasTextureField, cur_layer).tex.flip_v = enable_flag
        return self

    def flip_h(self, enable_flag: bool = True) -> 'TextureTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(HasTextureField, cur_layer).tex.flip_h = enable_flag
        return self


''' Media Concept '''


@dataclass
class MediaField:
    src: str
    gain: float = 1.0
    start: sec = sec(0)  # temporary


@runtime_checkable
class HasMediaField(tp.Protocol):
    media: MediaField


@dataclass
class MediaTrait:

    _idx: int

    def gain(self, gain: float) -> 'MediaTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(HasMediaField, cur_layer).media.gain = gain
        return self

    def start(self, start: sec | float) -> 'MediaTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(HasMediaField, cur_layer).media.start = sec(start)
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
        cur_unit_layer.unit.layer_indices.append(cur_layer_idx)

    return cur_layer_idx
