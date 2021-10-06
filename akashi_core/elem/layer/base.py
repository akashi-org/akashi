from __future__ import annotations
from dataclasses import dataclass
from abc import abstractmethod, ABCMeta
import typing as tp

from akashi_core.elem.context import _GlobalKronContext as gctx
from akashi_core.elem.uuid import UUID, gen_uuid
from akashi_core.time import sec
from akashi_core.pysl import FragShader, GeomShader


LayerKind = tp.Literal['LAYER', 'VIDEO', 'AUDIO', 'TEXT', 'IMAGE', 'FREE']


''' Layer Concept '''


@dataclass
class LayerField:
    uuid: UUID = UUID('')
    atom_uuid: UUID = UUID('')
    kind: LayerKind = 'LAYER'
    key: str = ''


class LayerTrait(metaclass=ABCMeta):
    ...


''' Position Concept '''


@dataclass
class PositionField:
    pos: tuple[int, int] = (0, 0)


class PositionTrait(metaclass=ABCMeta):
    @abstractmethod
    def pos(self, x: int, y: int) -> PositionTrait:
        ...


''' Duration Concept '''


@dataclass
class DurationField:
    duration: sec = sec(0)
    atom_offset: sec = sec(0)


class DurationTrait(metaclass=ABCMeta):
    @abstractmethod
    def duration(self, duration: sec) -> DurationTrait:
        ...

    @abstractmethod
    def offset(self, offset: sec) -> DurationTrait:
        ...


''' Shader Concept '''


@dataclass
class ShaderField:
    frag_shader: tp.Optional[FragShader] = None
    geom_shader: tp.Optional[GeomShader] = None


class ShaderTrait(metaclass=ABCMeta):
    @abstractmethod
    def frag(self, frag_shader: FragShader) -> ShaderTrait:
        ...

    @abstractmethod
    def geom(self, geom_shader: GeomShader) -> ShaderTrait:
        ...


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

    return cur_layer_idx