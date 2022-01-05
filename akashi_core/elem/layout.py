# pyright: reportPrivateUsage=false
from __future__ import annotations
import typing as tp

from .layer.base import PositionField
from .context import width as ak_width
from .context import height as ak_height


LayoutInfo: tp.TypeAlias = PositionField

LayoutFn: tp.TypeAlias = tp.Callable[[int, int], LayoutInfo]


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
