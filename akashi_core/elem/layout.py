# pyright: reportPrivateUsage=false
from __future__ import annotations
import typing as tp

from .layer.base import PositionField
from .context import width as ak_width
from .context import height as ak_height


LayoutInfo: tp.TypeAlias = PositionField

LayoutFn: tp.TypeAlias = tp.Callable[[int, int], LayoutInfo]


def vstack(item_width: int = -1) -> LayoutFn:
    def layout(lane_idx: int, lane_size: int) -> LayoutInfo:
        pos = (ak_width() // 2, int((ak_height() // lane_size) * (lane_idx + 0.5)))
        layer_size = (item_width, (ak_height() // lane_size))
        return LayoutInfo(pos, -1, layer_size)
    return layout


def hstack(item_height: int = -1) -> LayoutFn:
    def layout(lane_idx: int, lane_size: int) -> LayoutInfo:
        pos = (int((ak_width() // lane_size) * (lane_idx + 0.5)), ak_height() // 2)
        layer_size = ((ak_width() // lane_size), item_height)
        return LayoutInfo(pos, -1, layer_size)
    return layout
