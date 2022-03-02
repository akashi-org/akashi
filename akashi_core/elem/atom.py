# pyright: reportPrivateUsage=false
from __future__ import annotations
from dataclasses import dataclass, field
import typing as tp
from .context import _GlobalKronContext as gctx
from .uuid import gen_uuid, UUID
from akashi_core.time import sec
from akashi_core.color import Color as ColorEnum
from akashi_core.color import color_value
from akashi_core.probe import get_duration, g_resource_map

if tp.TYPE_CHECKING:
    from .layer.base import LayerField


@dataclass
class AtomEntry:

    uuid: UUID
    layer_indices: list[int] = field(default_factory=list, init=False)
    bg_color: str = "#000000"  # "#rrggbb"
    _duration: sec = field(default=sec(0), init=False)


@dataclass
class AtomHandle:

    _atom_idx: int

    def __enter__(self) -> 'AtomHandle':
        return self

    def __exit__(self, *ext: tp.Any):

        cur_atom = gctx.get_ctx().atoms[self._atom_idx]
        cur_layers = gctx.get_ctx().layers
        max_to: sec = sec(0)
        atom_fitted_layer_indices: list[int] = []
        for layer_idx in cur_atom.layer_indices:
            if isinstance(cur_layers[layer_idx].duration, sec):

                # resolve -1 duration
                if cur_layers[layer_idx].kind in ["VIDEO", "AUDIO"] and cur_layers[layer_idx].duration == sec(-1):
                    layer_src: str = cur_layers[layer_idx].src  # type: ignore
                    if layer_src in g_resource_map:
                        cur_layers[layer_idx].duration = g_resource_map[layer_src]
                    else:
                        cur_layers[layer_idx].duration = get_duration(layer_src)
                        g_resource_map[layer_src] = tp.cast(sec, cur_layers[layer_idx].duration)

                layer_to = cur_layers[layer_idx].atom_offset + tp.cast(sec, cur_layers[layer_idx].duration)
                if layer_to > max_to:
                    max_to = layer_to
            else:
                atom_fitted_layer_indices.append(layer_idx)

        cur_atom._duration = max_to

        # resolve atom fitted layers
        for at_layer_idx in atom_fitted_layer_indices:
            cur_layers[at_layer_idx].duration = cur_atom._duration

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
