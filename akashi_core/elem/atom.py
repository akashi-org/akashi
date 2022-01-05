# pyright: reportPrivateUsage=false
from __future__ import annotations
from dataclasses import dataclass, field
import typing as tp
from .context import _GlobalKronContext as gctx
from .uuid import gen_uuid, UUID
from akashi_core.time import sec

from .layer.base import PositionField

if tp.TYPE_CHECKING:
    from .lane import LaneEntry
    from .layer.base import LayerField
    from .layout import LayoutFn


@dataclass
class AtomEntry:

    uuid: UUID
    layer_indices: list[int] = field(default_factory=list, init=False)
    _lanes: list['LaneEntry'] = field(default_factory=list, init=False)
    _on_lane: bool = field(default=False, init=False)
    _duration: sec = field(default=sec(0), init=False)
    # [TODO] we should use layer index
    _atom_fitted_layers: list['LayerField'] = field(default_factory=list, init=False)
    _layout_lane_idx: int = field(default=-1, init=False)
    _layout_fn: 'LayoutFn' | None = field(default=None, init=False)


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

    def begin_layout(self, layout_fn: 'LayoutFn') -> None:
        cur_atom = gctx.get_ctx().atoms[self._atom_idx]
        cur_atom._layout_fn = layout_fn
        cur_atom._layout_lane_idx = len(cur_atom._lanes)

    def end_layout(self) -> None:

        cur_atom = gctx.get_ctx().atoms[self._atom_idx]

        if not cur_atom._layout_fn:
            raise Exception('Layout function is null')

        layout_lanes = cur_atom._lanes[cur_atom._layout_lane_idx:]
        for idx, layout_lane in enumerate(layout_lanes):
            layout_info = cur_atom._layout_fn(idx, len(layout_lanes))
            for item in layout_lane.items:
                if isinstance(item, PositionField):
                    item.pos = layout_info.pos
                    item.z = layout_info.z
                    item.layer_size = layout_info.layer_size

        cur_atom._layout_fn = None
        cur_atom._layout_lane_idx = -1


def atom() -> AtomHandle:

    uuid = gen_uuid()
    _atom = AtomEntry(uuid)
    gctx.get_ctx().atoms.append(_atom)
    atom_idx = len(gctx.get_ctx().atoms) - 1

    return AtomHandle(atom_idx)
