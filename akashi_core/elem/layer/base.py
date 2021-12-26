# pyright: reportPrivateUsage=false
from __future__ import annotations
from dataclasses import dataclass
from abc import ABCMeta
import typing as tp

from akashi_core.elem.context import _GlobalKronContext as gctx
from akashi_core.elem.uuid import UUID, gen_uuid
from akashi_core.time import sec
from akashi_core.pysl.shader import FragShader, PolygonShader

if tp.TYPE_CHECKING:
    from akashi_core.elem.atom import AtomHandle
    from akashi_core.pysl.shader import _GEntryFragFn, _GEntryPolyFn


LayerKind = tp.Literal['LAYER', 'VIDEO', 'AUDIO', 'TEXT', 'IMAGE', 'EFFECT', 'SHAPE', 'FREE']


''' Layer Concept '''

_TLayerTrait = tp.TypeVar('_TLayerTrait', bound='LayerTrait')


@dataclass
class LayerField:
    uuid: UUID = UUID('')
    atom_uuid: UUID = UUID('')
    kind: LayerKind = 'LAYER'
    key: str = ''
    duration: tp.Union[sec, 'AtomHandle'] = sec(0)
    atom_offset: sec = sec(0)


@dataclass
class LayerTrait(metaclass=ABCMeta):

    _idx: int

    def __enter__(self):
        return self

    def __exit__(self, *ext: tp.Any):
        return False

    def duration(self: '_TLayerTrait', duration: sec) -> '_TLayerTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(LayerField, cur_layer).duration = duration
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


class PositionTrait(LayerTrait, metaclass=ABCMeta):
    def pos(self: '_TPositionTrait', x: int, y: int) -> '_TPositionTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(PositionField, cur_layer).pos = (x, y)
        return self

    def z(self: '_TPositionTrait', value: float) -> '_TPositionTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(PositionField, cur_layer).z = value
        return self


''' Shader Concept '''

_TShaderTrait = tp.TypeVar('_TShaderTrait', bound='ShaderTrait')


@dataclass
class ShaderField:
    frag_shader: tp.Optional[FragShader] = None
    poly_shader: tp.Optional[PolygonShader] = None


class ShaderTrait(LayerTrait, metaclass=ABCMeta):

    def frag(self: '_TShaderTrait', *frag_shaders: '_GEntryFragFn') -> '_TShaderTrait':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, ShaderField):
            cur_layer.frag_shader = FragShader(frag_shaders)
        return self

    def poly(self: '_TShaderTrait', *poly_shaders: '_GEntryPolyFn') -> '_TShaderTrait':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, ShaderField):
            cur_layer.poly_shader = PolygonShader(poly_shaders)
        return self


''' FittableDuration Concept '''

_TFittableDurationTrait = tp.TypeVar('_TFittableDurationTrait', bound='FittableDurationTrait')


class FittableDurationTrait(LayerTrait, metaclass=ABCMeta):
    def fit_to(self: '_TFittableDurationTrait', handle: 'AtomHandle') -> '_TFittableDurationTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(LayerField, cur_layer).duration = handle
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
    cur_atom.layer_indices.append(cur_layer_idx)

    if len(cur_atom._lanes) == 0:
        raise Exception('Layer initialization outside the lane is prohibited')
    cur_atom._lanes[-1].items.append(cur_ctx.layers[-1])

    return cur_layer_idx
