# pyright: reportPrivateUsage=false
from __future__ import annotations
import typing as tp
from dataclasses import dataclass, field

from .layer.base import PositionField
from .context import _GlobalKronContext as gctx
from .context import width as ak_width
from .context import height as ak_height


LayoutInfo: tp.TypeAlias = PositionField

# (lane_idx: int, lane_size: int) => LayoutInfo
LayoutFn: tp.TypeAlias = tp.Callable[[int, int], LayoutInfo]


@dataclass
class AtomLayoutHandle:

    __layout_fn: 'LayoutFn'
    __layout_lane_idx: int = field(default=-1, init=False)

    def __enter__(self):
        self.begin()

    def __exit__(self, *ext: tp.Any):
        self.end()
        return False

    def begin(self) -> None:
        cur_atom = gctx.get_ctx().atoms[-1]
        self.__layout_lane_idx = len(cur_atom._lanes)

    def end(self) -> None:

        cur_atom = gctx.get_ctx().atoms[-1]

        layout_lanes = cur_atom._lanes[self.__layout_lane_idx:]
        for idx, layout_lane in enumerate(layout_lanes):
            layout_info = self.__layout_fn(idx, len(layout_lanes))
            for item in layout_lane.items:
                if isinstance(item, PositionField):
                    item.pos = layout_info.pos
                    item.z = layout_info.z
                    item.layer_size = layout_info.layer_size


def vstack(item_width: int = -1, layout_height: int | None = None, item_offsets: tuple[int, int] = (0, 0)) -> LayoutFn:
    if not layout_height:
        layout_height = ak_height()

    def layout(lane_idx: int, lane_size: int) -> LayoutInfo:
        px = (ak_width() // 2) + item_offsets[0]
        py = int((layout_height // lane_size) * (lane_idx + 0.5)) + \
            ((ak_height() - layout_height) // 2) + item_offsets[1]
        layer_size = (item_width, (layout_height // lane_size))
        return LayoutInfo((px, py), -1, layer_size)
    return layout


def hstack(item_height: int = -1, layout_width: int | None = None, item_offsets: tuple[int, int] = (0, 0)) -> LayoutFn:
    if not layout_width:
        layout_width = ak_width()

    def layout(lane_idx: int, lane_size: int) -> LayoutInfo:
        px = int((layout_width // lane_size) * (lane_idx + 0.5)) + ((ak_width() - layout_width) // 2) + item_offsets[0]
        py = (ak_height() // 2) + item_offsets[1]
        layer_size = ((layout_width // lane_size), item_height)
        return LayoutInfo((px, py), -1, layer_size)
    return layout


def layout(layout_fn: LayoutFn) -> AtomLayoutHandle:

    return AtomLayoutHandle(layout_fn)
