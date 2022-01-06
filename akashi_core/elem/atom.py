# pyright: reportPrivateUsage=false
from __future__ import annotations
from dataclasses import dataclass, field
import typing as tp
from .context import _GlobalKronContext as gctx
from .uuid import gen_uuid, UUID
from akashi_core.time import sec
from akashi_core.color import Color as ColorEnum
from akashi_core.color import color_value

if tp.TYPE_CHECKING:
    from .lane import LaneEntry
    from .layer.base import LayerField


@dataclass
class AtomEntry:

    uuid: UUID
    layer_indices: list[int] = field(default_factory=list, init=False)
    bg_color: str = "#000000"  # "#rrggbb"
    _lanes: list['LaneEntry'] = field(default_factory=list, init=False)
    _on_lane: bool = field(default=False, init=False)
    _duration: sec = field(default=sec(0), init=False)
    # [TODO] we should use layer index

    _atom_fitted_layers: list['LayerField'] = field(default_factory=list, init=False)


@dataclass
class AtomHandle:

    _atom_idx: int

    def __enter__(self) -> 'AtomHandle':
        return self

    def __exit__(self, *ext: tp.Any):

        cur_atom = gctx.get_ctx().atoms[self._atom_idx]
        for at_layer in cur_atom._atom_fitted_layers:
            at_layer.duration = cur_atom._duration

        return False

    def bg_color(self, color: tp.Union[str, 'ColorEnum']) -> 'AtomHandle':
        cur_atom = gctx.get_ctx().atoms[self._atom_idx]
        cur_atom.bg_color = color_value(color)
        return self


def atom() -> AtomHandle:

    uuid = gen_uuid()
    _atom = AtomEntry(uuid)
    gctx.get_ctx().atoms.append(_atom)
    atom_idx = len(gctx.get_ctx().atoms) - 1

    return AtomHandle(atom_idx)
