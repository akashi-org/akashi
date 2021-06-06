from __future__ import annotations
from typing import (
    Tuple,
    Any,
    Literal
)
from .time import Second
from dataclasses import dataclass, asdict
from os import path
import json


"""
  data definitions
"""


@dataclass(frozen=True)
class GenerelConf:
    entry_file: str
    include_dir: str


@dataclass(frozen=True)
class VideoConf:
    fps: Second = Second(24)
    resolution: Tuple[int, int] = (1920, 1080)
    default_font_path: str = "/usr/share/fonts/truetype/freefont/FreeSans.ttf"


AudioSampleFormat = Literal['', 'u8', 's16', 's32', 'flt', 'dbl']


AudioChannelLayout = Literal['', 'mono', 'stereo']


@dataclass(frozen=True)
class AudioConf:
    format: AudioSampleFormat = 'flt'
    sample_rate: int = 44100
    channels: int = 2
    channel_layout: AudioChannelLayout = 'stereo'


VideoDecodeMethod = Literal['', 'sw', 'vaapi', 'vaapi_copy']


@dataclass(frozen=True)
class PlaybackConf:
    enable_loop: bool = True
    gain: float = 0.5  # 0 ~ 1.0
    decode_method: VideoDecodeMethod = 'sw'
    video_max_queue_size: int = 1024 * 1024 * 300  # 300mb
    video_max_queue_count: int = 64  # max frame counts (applicable for hwdec)
    audio_max_queue_size: int = 1024 * 1024 * 10  # 10mb


WindowMode = Literal['', 'split', 'immersive', 'independent']


@dataclass(frozen=True)
class UIConf:
    resolution: Tuple[int, int] = (800, 600)
    window_mode: WindowMode = 'split'
    smart_immersive: bool = False


VideoEncodeMethod = Literal['', 'sw']


@dataclass(frozen=True)
class EncodeConf:
    out_fname: str = ''
    video_codec: str = ''
    audio_codec: str = ''
    encode_max_queue_count: int = 10  # max queue element counts
    encode_method: VideoEncodeMethod = 'sw'


@dataclass(frozen=True)
class AKConf:
    general: GenerelConf
    video: VideoConf = VideoConf()
    audio: AudioConf = AudioConf()
    playback: PlaybackConf = PlaybackConf()
    ui: UIConf = UIConf()
    encode: EncodeConf = EncodeConf()

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
