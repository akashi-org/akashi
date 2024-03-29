from __future__ import annotations
from typing import (
    Tuple,
    Any,
    Literal,
    Callable
)
from .time import sec
from dataclasses import dataclass, asdict, field
from os import path
import json
import sys

from akashi_cli.config_parser import config_parse


"""
  data definitions
"""


@dataclass(frozen=True)
class GenerelConf:
    entry_file: str
    include_dir: str


VideoDecodeMethod = Literal['', 'sw', 'vaapi', 'vaapi_copy']


@dataclass(frozen=True)
class VideoConf:
    fps: sec = field(default_factory=lambda: sec(24))
    resolution: Tuple[int, int] = (1920, 1080)
    default_font_path: str = "/usr/share/fonts/truetype/freefont/FreeSans.ttf"
    msaa: int = 1  # msaa >= 1
    preferred_decode_method: VideoDecodeMethod = 'vaapi'
    vaapi_device: str = ''  # ex. /dev/dri/renderD128


AudioSampleFormat = Literal['', 'u8', 's16', 's32', 'flt', 'dbl']


AudioChannelLayout = Literal['', 'mono', 'stereo']


@dataclass(frozen=True)
class AudioConf:
    format: AudioSampleFormat = 'flt'
    sample_rate: int = 44100
    channels: int = 2
    channel_layout: AudioChannelLayout = 'stereo'


@dataclass(frozen=True)
class PlaybackConf:
    gain: float = 0.5  # 0 ~ 1.0
    video_max_queue_size: int = 1024 * 1024 * 300  # 300mb
    video_max_queue_count: int = 64  # max frame counts (applicable for hwdec)
    audio_max_queue_size: int = 1024 * 1024 * 10  # 10mb


WindowMode = Literal['', 'split', 'immersive', 'independent']


@dataclass(frozen=True)
class UIConf:
    resolution: Tuple[int, int] = (800, 600)
    window_mode: WindowMode = 'independent'
    smart_immersive: bool = False
    frameless_window: bool = True

    def __post_init__(self):

        w, h = self.resolution
        if h > w:
            raise Exception('`resolution[0]` in UIConf must not be lower than `resolution[1]`')


VideoEncodeMethod = Literal['', 'sw', 'vaapi', 'vaapi_copy']


@dataclass(frozen=True)
class EncodeConf:
    video_codec: str = ''
    audio_codec: str = ''
    ffmpeg_format_opts: str = ''
    video_ffmpeg_codec_opts: str = ''
    audio_ffmpeg_codec_opts: str = ''
    encode_max_queue_count: int = 10  # max queue element counts
    encode_method: VideoEncodeMethod = 'sw'
    out_fname: str = field(default='', init=False)


@dataclass(frozen=True)
class AKConf:
    general: GenerelConf
    video: VideoConf
    audio: AudioConf
    playback: PlaybackConf
    ui: UIConf
    encode: EncodeConf

    def to_json(self) -> str:

        class AKConfEncoder(json.JSONEncoder):
            def default(self, o: Any) -> Any:
                if hasattr(o, 'to_json'):
                    return o.to_json()
                else:
                    return json.JSONEncoder.default(self, o)

        return json.dumps(asdict(self), cls=AKConfEncoder, ensure_ascii=False)


"""
  utilities for configuration
"""


def from_relpath(bpath: str, tpath: str) -> str:
    return path.normpath(path.join(path.dirname(path.abspath(bpath)), tpath))


ConfFn = Callable[[], AKConf]


def config() -> Callable[[ConfFn], ConfFn]:
    def inner(fn: ConfFn):
        sys.modules[fn.__module__].__akashi_export_config_fn = fn  # type: ignore
        return fn
    return inner
