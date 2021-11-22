# pyright: reportPrivateUsage=false
from __future__ import annotations
from dataclasses import dataclass, field
import typing as tp
from .context import _GlobalKronContext as gctx
from .uuid import gen_uuid, UUID
from akashi_core.time import sec

if tp.TYPE_CHECKING:
    from .lane import LaneEntry


@dataclass
class AtomEntry:

    uuid: UUID
    layer_indices: list[int] = field(default_factory=list, init=False)
    _lanes: list[LaneEntry] = field(default_factory=list, init=False)
    _on_lane: bool = field(default=False, init=False)


@dataclass
class AtomHandle:

    _atom_idx: int

    def __enter__(self):
        return self

    def __exit__(self, *ext: tp.Any):
        return False


def atom() -> AtomHandle:

    uuid = gen_uuid()
    _atom = AtomEntry(uuid)
    gctx.get_ctx().atoms.append(_atom)
    atom_idx = len(gctx.get_ctx().atoms) - 1

    return AtomHandle(atom_idx)
