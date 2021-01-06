from __future__ import annotations
from typing import (
    Union,
    Callable,
    Generic,
    TypeVar,
    Optional,
    Sequence,
    Literal,
    TypedDict,
)
from .time import Second
from dataclasses import dataclass, field


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
    x: int
    y: int
    _type: Literal['LAYER'] = field(default='LAYER', init=False)
    _uuid: str = field(default='', init=False)
    _atom_uuid: str = field(default='')
    _display: bool = field(default=False, init=False)


@dataclass
class _VideoLayerRequiredParams:
    src: str


@dataclass
class VideoLayerParams(CommonLayerParams, _VideoLayerRequiredParams):
    _type: Literal['VIDEO'] = field(default='VIDEO', init=False)
    start: Second = field(default=Second(0))
    frag_path: str = field(default="")
    geom_path: str = field(default="")


@dataclass
class _AudioLayerRequiredParams:
    src: str


@dataclass
class AudioLayerParams(CommonLayerParams, _AudioLayerRequiredParams):
    _type: Literal['AUDIO'] = field(default='AUDIO', init=False)
    start: Second = field(default=Second(0))


class TextLayerStyle(TypedDict, total=False):
    font_size: int
    font_path: str
    fill: str  # rrggbb


@dataclass
class _TextLayerRequiredParams:
    text: str


@dataclass
class TextLayerParams(CommonLayerParams, _TextLayerRequiredParams):
    _type: Literal['TEXT'] = field(default='TEXT', init=False)
    style: TextLayerStyle = field(default_factory=TextLayerStyle)
    frag_path: str = field(default="")
    geom_path: str = field(default="")


@dataclass
class _ImageLayerRequiredParams:
    src: str


@dataclass
class ImageLayerParams(CommonLayerParams, _ImageLayerRequiredParams):
    _type: Literal['IMAGE'] = field(default='IMAGE', init=False)
    frag_path: str = field(default="")
    geom_path: str = field(default="")


LayerParams = Union[
    CommonLayerParams,
    VideoLayerParams,
    AudioLayerParams,
    TextLayerParams,
    ImageLayerParams
]

TLayerParams = TypeVar("TLayerParams", bound=CommonLayerParams)

TLayerParams_contra = TypeVar("TLayerParams_contra", bound=CommonLayerParams, contravariant=True)

TLayerParams_co = TypeVar("TLayerParams_co", bound=CommonLayerParams, covariant=True)

# LayerInitFunc = Callable[[], _TLayerParams]

LayerUpdateFunc = Callable[[KronArgs, TLayerParams], TLayerParams]


@dataclass(frozen=True)
class LayerReceiver(Generic[TLayerParams_contra]):
    mod_params: Optional[Callable[[TLayerParams_contra], TLayerParams_contra]] = None
    get_finalized_params: Optional[Callable[[TLayerParams_contra], None]] = None
    get_frame_cnt: Optional[Callable[[int], None]] = None
    get_duration: Optional[Callable[[Second], None]] = None


@dataclass(frozen=True)
class FrameContext:
    pts: Second
    # layer_ctxs: LayerParams<T>[];
    layer_ctxs: Sequence[LayerParams]


FrameContextGen = Callable[[KronArgs], FrameContext]

LayerContext = TLayerParams_co

LayerContextGen = Callable[[KronArgs], LayerContext]

CommonLayerType = Callable[[LayerReceiver[CommonLayerParams]], LayerContextGen[CommonLayerParams]]

VideoLayerType = Callable[[LayerReceiver[VideoLayerParams]], LayerContextGen[VideoLayerParams]]

AudioLayerType = Callable[[LayerReceiver[AudioLayerParams]], LayerContextGen[AudioLayerParams]]

TextLayerType = Callable[[LayerReceiver[TextLayerParams]], LayerContextGen[TextLayerParams]]

ImageLayerType = Callable[[LayerReceiver[ImageLayerParams]], LayerContextGen[ImageLayerParams]]

LayerType = Union[
    CommonLayerType,
    VideoLayerType,
    AudioLayerType,
    TextLayerType,
    ImageLayerType
]

TLayerType = TypeVar("TLayerType", bound=CommonLayerType)


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


KronParams = Union[
    RootParams,
    SceneParams,
    AtomParams,
]


_TKronParams = TypeVar('_TKronParams', RootParams, SceneParams, AtomParams)

TKronChild_co = TypeVar('TKronChild_co', covariant=True)


@dataclass(frozen=True)
class KronReceiver(Generic[_TKronParams, TKronChild_co]):
    get_children: Optional[Callable[[Sequence[TKronChild_co]], None]] = None
    mod_params: Optional[Callable[[_TKronParams, Sequence[TKronChild_co]], _TKronParams]] = None
    get_finalized_params: Optional[Callable[[_TKronParams], None]] = None
    get_frame_cnt: Optional[Callable[[int], None]] = None
    get_duration: Optional[Callable[[Second], None]] = None


AtomReceiver = KronReceiver[AtomParams, LayerType]

AtomType = Callable[[AtomReceiver], FrameContextGen]

SceneReceiver = KronReceiver[SceneParams, AtomType]

SceneType = Callable[[SceneReceiver], FrameContextGen]

RootReceiver = KronReceiver[RootParams, SceneType]

RootType = Callable[[RootReceiver], FrameContextGen]

KronType = Union[RootType, SceneType, AtomType]

TKronType = TypeVar("TKronType", RootType, SceneType, AtomType)

KronChildType = Union[SceneType, AtomType, LayerType]
