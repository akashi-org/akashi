# pyright: reportPrivateUsage=false
from __future__ import annotations
import typing as tp
from dataclasses import dataclass, field

from .context import _GlobalKronContext as gctx
from .context import lwidth as ak_lwidth
from .context import lheight as ak_lheight


@dataclass
class LayoutLayerContext:

    key: str
    idx: int  # layer index
    num_layers: int  # max number of layers in atom or unit


@dataclass
class LayoutInfo:
    pos: tuple[int, int] = (0, 0)
    z: float = 0.0
    layer_size: tuple[int, int] = (-1, -1)


LayoutFn: tp.TypeAlias = tp.Callable[[LayoutLayerContext], LayoutInfo | None]


def vstack(item_width: int = -1, layout_height: int | None = None, item_offsets: tuple[int, int] = (0, 0)) -> LayoutFn:
    if not layout_height:
        layout_height = ak_lheight()

    def layout(layer_ctx: LayoutLayerContext) -> LayoutInfo:
        px = (ak_lwidth() // 2) + item_offsets[0]
        py = int((layout_height // layer_ctx.num_layers) * (layer_ctx.idx + 0.5)) + \
            ((ak_lheight() - layout_height) // 2) + item_offsets[1]
        layer_size = (item_width, (layout_height // layer_ctx.num_layers))
        return LayoutInfo(pos=(px, py), layer_size=layer_size)
    return layout


def hstack(item_height: int = -1, layout_width: int | None = None, item_offsets: tuple[int, int] = (0, 0)) -> LayoutFn:
    if not layout_width:
        layout_width = ak_lwidth()

    def layout(layer_ctx: LayoutLayerContext) -> LayoutInfo:
        px = int((layout_width // layer_ctx.num_layers) * (layer_ctx.idx + 0.5)) + \
            ((ak_lwidth() - layout_width) // 2) + item_offsets[0]
        py = (ak_lheight() // 2) + item_offsets[1]
        layer_size = ((layout_width // layer_ctx.num_layers), item_height)
        return LayoutInfo(pos=(px, py), layer_size=layer_size)
    return layout
