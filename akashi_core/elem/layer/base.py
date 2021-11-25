# pyright: reportPrivateUsage=false
from __future__ import annotations
from dataclasses import dataclass
from abc import abstractmethod, ABCMeta
import typing as tp

from akashi_core.elem.context import _GlobalKronContext as gctx
from akashi_core.elem.uuid import UUID, gen_uuid
from akashi_core.time import sec
from akashi_core.pysl import FragShader, PolygonShader

if tp.TYPE_CHECKING:
    from akashi_core.elem.atom import AtomHandle


LayerKind = tp.Literal['LAYER', 'VIDEO', 'AUDIO', 'TEXT', 'IMAGE', 'EFFECT', 'FREE']


''' Layer Concept '''


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

    def duration(self, duration: sec):
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(LayerField, cur_layer).duration = duration
        return self


''' Position Concept '''


@dataclass
class PositionField:
    pos: tuple[int, int] = (0, 0)


class PositionTrait(LayerTrait, metaclass=ABCMeta):
    def pos(self, x: int, y: int):
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(PositionField, cur_layer).pos = (x, y)
        return self


''' Shader Concept '''


@dataclass
class ShaderField:
    frag_shader: tp.Optional[FragShader] = None
    poly_shader: tp.Optional[PolygonShader] = None


class ShaderTrait(LayerTrait, metaclass=ABCMeta):
    def frag(self, frag_shader: FragShader):
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(ShaderField, cur_layer).frag_shader = frag_shader
        return self

    def poly(self, poly_shader: PolygonShader):
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(ShaderField, cur_layer).poly_shader = poly_shader
        return self


''' FittableDuration Concept '''


class FittableDurationTrait(LayerTrait, metaclass=ABCMeta):
    def fit_to(self, handle: 'AtomHandle'):
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(LayerField, cur_layer).duration = handle
        return self


def __is_atom_active(atom_uuid: UUID, raise_exp: bool = True) -> bool:

    cur_atom = gctx.get_ctx().atoms[-1]
    r = atom_uuid == cur_atom.uuid
    if raise_exp and not(r):
        raise Exception('Update for the inactive atom is forbidden')
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
