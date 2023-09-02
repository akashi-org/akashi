# pyright: reportPrivateUsage=false
from __future__ import annotations
from dataclasses import dataclass, field
import typing as tp
import math
from akashi_core.time import sec, root_time_tp, root_time
from akashi_core.elem.context import _GlobalKronContext as gctx
from akashi_core.probe import get_duration

from .base import (
    _BaseTrait,
    _BaseTraitField,
    LayerRef,
)


''' Media Concept '''


@dataclass(unsafe_hash=True)
class _BaseMediaField(_BaseTraitField):
    req_src: str = ''
    gain: float = 1.0
    start: sec = field(default_factory=lambda: sec(0))
    end: sec = field(default_factory=lambda: sec(-1))
    _span_cnt: int | None = None
    _span_dur: None | sec | root_time_tp = None


@dataclass
class _BaseMediaTrait(_BaseTrait):

    _name: str = "base_media"

    def gain(self, gain: float) -> tp.Self:
        self._priv.get_trait_field(self).gain = gain
        return self

    def range(self, start: sec | float, end: sec | float = -1) -> tp.Self:
        if start < 0:
            raise Exception('Negative start value is prohibited')

        self._priv.get_trait_field(self).start = sec(start)
        self._priv.get_trait_field(self).end = sec(end)
        return self

    def span_cnt(self, count: int) -> tp.Self:
        self._priv.get_trait_field(self)._span_cnt = count
        self._priv.get_trait_field(self)._span_dur = None
        return self

    def span_dur(self, duration: sec | float | 'LayerRef' | root_time_tp) -> tp.Self:
        _duration: sec | root_time_tp
        if isinstance(duration, LayerRef):
            _duration = gctx.get_ctx().layers[int(duration.value)]._duration
            if not isinstance(_duration, sec):
                if _duration != root_time:
                    raise Exception('Invalid LayerRef found')
        elif isinstance(duration, (int, float)):
            _duration = sec(duration)
        else:
            _duration = duration

        self._priv.get_trait_field(self)._span_cnt = None
        self._priv.get_trait_field(self)._span_dur = _duration

        return self


def calc_media_duration(media: _BaseMediaField) -> sec | root_time_tp:
    if media.end == sec(-1):
        media.end = get_duration(media.req_src)

    media_dur = media.end - media.start
    if media._span_cnt:
        return media_dur * media._span_cnt
    elif media._span_dur:
        return media._span_dur
    else:
        return media_dur


''' Video Concept '''


@dataclass(unsafe_hash=True)
class VideoField(_BaseMediaField):
    ...


@dataclass
class VideoTrait(_BaseMediaTrait):
    _name: str = "video"


''' Audio Concept '''


@dataclass(unsafe_hash=True)
class AudioField(_BaseMediaField):
    ...


@dataclass
class AudioTrait(_BaseMediaTrait):
    _name: str = "audio"


''' Image Concept '''


@dataclass(unsafe_hash=True)
class ImageField(_BaseTraitField):
    req_srcs: tuple[str, ...] = field(default_factory=tuple)

    # to be used by elem_parser
    def _req_srcs_to_list(self) -> list[str]:
        return list(self.req_srcs)


@dataclass
class ImageTrait(_BaseTrait):
    _name: str = "image"
