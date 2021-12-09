# pyright: reportPrivateUsage=false
from __future__ import annotations
from dataclasses import dataclass, field
import typing as tp
from .context import _GlobalKronContext as gctx
from .uuid import gen_uuid, UUID
from akashi_core.time import sec
from akashi_core.probe import get_duration, g_resource_map

if tp.TYPE_CHECKING:
    from .layer.base import LayerField
    from .layer.video import VideoEntry
    from .layer.audio import AudioEntry


@dataclass
class LanePad:
    pad_sec: sec = sec(0)


@dataclass
class LaneEntry:

    items: list[tp.Union[LayerField, LanePad]] = field(default_factory=list)


@dataclass
class LaneHandle:

    __lane_idx: int = field(default=-1, init=False)

    def __enter__(self) -> 'LaneHandle':
        cur_atom = gctx.get_ctx().atoms[-1]
        if cur_atom._on_lane:
            raise Exception('Nested lanes is prohibited')
        cur_atom._on_lane = True
        cur_atom._lanes.append(LaneEntry())
        self.__lane_idx = len(cur_atom._lanes) - 1
        return self

    def __exit__(self, *ext: tp.Any):
        cur_atom = gctx.get_ctx().atoms[-1]
        cur_atom._on_lane = False
        cur_entry = cur_atom._lanes[self.__lane_idx]
        acc_duration: sec = sec(0)

        for item in cur_entry.items:
            if isinstance(item, LanePad):
                acc_duration += item.pad_sec
            # assumes LayerField
            else:
                # assumes AtomHandle
                if not isinstance(item.duration, sec):
                    if len(cur_entry.items) > 1:
                        raise Exception('Single item is only allowed in a lane with atom-fitted layer')
                    else:
                        cur_atom._atom_fitted_layers.append(item)
                        acc_duration = sec(0)
                        break

                item.atom_offset = acc_duration

                if item.kind in ["VIDEO", "AUDIO"] and item.duration == sec(-1):
                    # item_src = tp.cast(tp.Union[VideoEntry, AudioEntry], item).src
                    item_src: str = item.src  # type: ignore
                    if item_src in g_resource_map:
                        item.duration = g_resource_map[item_src]
                    else:
                        item.duration = get_duration(item_src)
                        g_resource_map[item_src] = item.duration

                acc_duration += item.duration

        if acc_duration > cur_atom._duration:
            cur_atom._duration = acc_duration

        return False

    def pad(self, pad_sec: sec) -> None:
        cur_atom = gctx.get_ctx().atoms[-1]
        cur_atom._lanes[self.__lane_idx].items.append(LanePad(pad_sec))


def lane() -> LaneHandle:

    return LaneHandle()
