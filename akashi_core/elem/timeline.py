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
class TimelinePad:
    pad_sec: sec = sec(0)


@dataclass
class TimelineEntry:

    key: str = field(default='')
    items: list[tp.Union[LayerField, TimelinePad]] = field(default_factory=list)


@dataclass
class TimelineHandle:

    __key: str = field(default='')

    def __enter__(self) -> 'TimelineHandle':
        if gctx.get_ctx()._cur_timeline:
            raise Exception('Nested timelines is prohibited')
        gctx.get_ctx()._cur_timeline = TimelineEntry(self.__key)
        return self

    def __exit__(self, *ext: tp.Any):
        cur_atom = gctx.get_ctx().atoms[-1]
        cur_entry = tp.cast(TimelineEntry, gctx.get_ctx()._cur_timeline)
        gctx.get_ctx()._cur_timeline = None
        acc_duration: sec = sec(0)

        for item in cur_entry.items:
            if isinstance(item, TimelinePad):
                acc_duration += item.pad_sec
            # assumes LayerField
            else:
                # assumes AtomHandle
                if not isinstance(item.duration, sec):
                    raise Exception('Atom fitted layer is not allowed in timeline')

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

        return False

    def pad(self, pad_sec: sec) -> None:
        if gctx.get_ctx()._cur_timeline:
            tp.cast(TimelineEntry, gctx.get_ctx()._cur_timeline).items.append(TimelinePad(pad_sec))


def timeline(key: str = '') -> TimelineHandle:

    return TimelineHandle(key)
