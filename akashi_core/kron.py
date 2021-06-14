# pyright: reportIncompatibleVariableOverride=false
from __future__ import annotations
from typing import (
    Union,
    Callable,
    TypeVar,
    Optional,
    Sequence,
    Literal,
    TypedDict,
)
from .time import Second
from .shader import EntryShader
from dataclasses import dataclass, field, fields, replace
from uuid import uuid4


@dataclass(frozen=True)
class KronArgs:
    playTime: Second
    fps: int


KronName = Literal['ROOT', 'SCENE', 'ATOM']

LayerKind = Literal['LAYER', 'VIDEO', 'AUDIO', 'TEXT', 'IMAGE']


@dataclass
class CommonLayerParams:
    begin: Second
    end: Second
    x: int = 0
    y: int = 0
    _type: Literal['LAYER'] = field(default='LAYER', init=False)
    _uuid: str = field(default='', init=False)
    _atom_uuid: str = field(default='')
    _display: bool = field(default=False, init=False)

    def _update(self, _begin: Second, _end: Second):
        new_params = replace(self, begin=_begin, end=_end)
        for fld in fields(self):
            if not fld.init:
                setattr(new_params, fld.name, getattr(self, fld.name))
        return new_params


@dataclass
class _VideoLayerRequiredParams:
    src: str


@dataclass
class VideoLayerParams(CommonLayerParams, _VideoLayerRequiredParams):
    _type: Literal['VIDEO'] = field(default='VIDEO', init=False)
    start: Second = field(default=Second(0))
    scale: float = field(default=1.0)
    gain: float = field(default=1.0)
    frag: Optional[EntryShader[Literal['frag'], Literal['video']]] = field(default=None)
    geom: Optional[EntryShader[Literal['geom'], Literal['video']]] = field(default=None)


@dataclass
class _AudioLayerRequiredParams:
    src: str


@dataclass
class AudioLayerParams(CommonLayerParams, _AudioLayerRequiredParams):
    _type: Literal['AUDIO'] = field(default='AUDIO', init=False)
    start: Second = field(default=Second(0))
    gain: float = field(default=1.0)


class TextLayerStyle(TypedDict, total=False):
    font_size: int
    font_path: str
    fill: str  # rrggbb


@dataclass
class _TextLayerRequiredParams:
    text: str


@dataclass
class TextLayerParams(CommonLayerParams, _TextLayerRequiredParams):
    scale: float = field(default=1.0)
    _type: Literal['TEXT'] = field(default='TEXT', init=False)
    style: TextLayerStyle = field(default_factory=TextLayerStyle)
    frag: Optional[EntryShader[Literal['frag'], Literal['novideo']]] = field(default=None)
    geom: Optional[EntryShader[Literal['geom'], Literal['novideo']]] = field(default=None)


@dataclass
class _ImageLayerRequiredParams:
    src: str


@dataclass
class ImageLayerParams(CommonLayerParams, _ImageLayerRequiredParams):
    _type: Literal['IMAGE'] = field(default='IMAGE', init=False)
    scale: float = field(default=1.0)
    frag: Optional[EntryShader[Literal['frag'], Literal['novideo']]] = field(default=None)
    geom: Optional[EntryShader[Literal['geom'], Literal['novideo']]] = field(default=None)


LayerParams = Union[
    CommonLayerParams,
    VideoLayerParams,
    AudioLayerParams,
    TextLayerParams,
    ImageLayerParams
]

_TLayerParams = TypeVar("_TLayerParams", bound=CommonLayerParams)

# LayerInitFunc = Callable[[], _TLayerParams]

LayerUpdateFunc = Callable[[KronArgs, _TLayerParams], _TLayerParams]


@dataclass(frozen=True)
class FrameContext:
    pts: Second
    # layer_ctxs: LayerParams<T>[];
    layer_ctxs: Sequence[LayerParams]


CommonLayerType = Callable[[], Literal['LAYER']]

VideoLayerType = Callable[[], Literal['VIDEO']]

AudioLayerType = Callable[[], Literal['AUDIO']]

TextLayerType = Callable[[], Literal['TEXT']]

ImageLayerType = Callable[[], Literal['IMAGE']]

LayerType = Union[
    CommonLayerType,
    VideoLayerType,
    AudioLayerType,
    TextLayerType,
    ImageLayerType
]


class RootInputParams(TypedDict, total=False):
    ...


@dataclass
class RootParams:
    _type: Literal['ROOT'] = field(default='ROOT', init=False)
    _uuid: str = field(default='', init=False)


class SceneInputParams(TypedDict, total=False):
    path: str


@dataclass
class SceneParams:
    path: Optional[str] = None
    _type: Literal['SCENE'] = field(default='SCENE', init=False)
    _duration: Second = field(default=Second(0), init=False)
    _offset: Second = field(default=Second(0))
    _frame_cnt: int = field(default=0, init=False)
    _uuid: str = field(default='', init=False)


class _AtomInputOptionalParams(TypedDict, total=False):
    duration: Second


class AtomInputParams(_AtomInputOptionalParams):
    ...


@dataclass
class AtomParams:
    duration: Second = Second(0)
    _type: Literal['ATOM'] = field(default='ATOM', init=False)
    _offset: Second = field(default=Second(0))
    _frame_cnt: int = field(default=0)
    _uuid: str = field(default='', init=False)


AtomType = Callable[[], Literal['ATOM']]

SceneType = Callable[[], Literal['SCENE']]

RootType = Callable[[], Literal['ROOT']]


def video(
    init: VideoLayerParams,
    update: Optional[LayerUpdateFunc[VideoLayerParams]] = None
) -> VideoLayerType:
    uuid = str(uuid4())

    def closure() -> Literal['VIDEO']:
        uuid, init, update
        return 'VIDEO'
    return closure


def audio(
    init: AudioLayerParams,
    update: Optional[LayerUpdateFunc[AudioLayerParams]] = None
) -> AudioLayerType:
    uuid = str(uuid4())

    def closure() -> Literal['AUDIO']:
        uuid, init, update
        return 'AUDIO'
    return closure


def text(
    init: TextLayerParams,
    update: Optional[LayerUpdateFunc[TextLayerParams]] = None
) -> TextLayerType:
    uuid = str(uuid4())

    def closure() -> Literal['TEXT']:
        uuid, init, update
        return 'TEXT'
    return closure


def image(
    init: ImageLayerParams,
    update: Optional[LayerUpdateFunc[ImageLayerParams]] = None
) -> ImageLayerType:
    uuid = str(uuid4())

    def closure() -> Literal['IMAGE']:
        uuid, init, update
        return 'IMAGE'
    return closure


def atom(params: AtomInputParams, children: list[LayerType]) -> AtomType:
    uuid = str(uuid4())

    def closure() -> Literal['ATOM']:
        uuid, params, children
        return 'ATOM'
    return closure


def scene(params: SceneInputParams, children: list[AtomType]) -> SceneType:
    uuid = str(uuid4())

    def closure() -> Literal['SCENE']:
        uuid, params, children
        return 'SCENE'
    return closure


def root(_: RootInputParams, children: list[SceneType]) -> RootType:
    uuid = str(uuid4())

    def closure() -> Literal['ROOT']:
        uuid, _, children
        return 'ROOT'
    return closure
