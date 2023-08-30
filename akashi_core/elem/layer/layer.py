# pyright: reportPrivateUsage=false
from __future__ import annotations
from dataclasses import dataclass, field, fields
import typing as tp
from typing import overload
import math

from akashi_core.elem.context import _GlobalKronContext as gctx, lcenter
from akashi_core.elem.context import lwidth as ak_lwidth, lheight as ak_lheight
from akashi_core.elem.uuid import UUID, gen_uuid
from akashi_core.time import sec, NOT_FIXED_SEC
from akashi_core.probe import get_duration

from .base import (
    register_layer_field,
    LayerRef,
    _BaseTrait,
    _BaseTraitField,
    TimeTrait,
    LayerField,
    LayerTrait,
    TimeTrait,
    TransformField,
    TransformTrait,
    ShaderField,
    ShaderTrait,
    TextureField,
    TextureTrait,
)

from .media import (
    _BaseMediaField,
    _BaseMediaTrait,
    VideoField,
    VideoTrait,
    AudioField,
    AudioTrait,
    ImageField,
    ImageTrait,
    calc_media_duration,
)

from .text import (
    TextField,
    TextTrait,
    TextStyleField,
    TextStyleTrait,
)

from .shape import (
    _BaseShapeField,
    _BaseShapeTrait,
    RectField,
    RectTrait,
    CircleField,
    CircleTrait,
    TriField,
    TriTrait,
    LineField,
    LineTrait
)

from .unit import (
    UnitField,
    UnitTrait,
    _FrameFnCtxOpaque,
    _FrameFnCtx,
    SpatialFrameGuard,
    TimelineFrameGuard,
)


def create_trait_field(trait_name: str) -> _BaseTraitField:

    if trait_name in ['layer', 'time']:
        raise Exception('Invalid trait name')

    # [XXX] There should be all trait fields in this module
    field_name = trait_name.capitalize() + 'Field'
    if trait_name == "text_style":
        field_name = "TextStyleField"
    return globals()[field_name]()


def create_trait(trait_name: str, priv: '_BaseTraitPriv') -> _BaseTrait:
    # [XXX] There should be all traits in this module
    _name = trait_name.split('t_')[-1].capitalize() + 'Trait'
    if trait_name == "t_text_style":
        _name = "TextStyleTrait"
    return globals()[_name](_priv=priv)


@dataclass
class _BaseTraitPriv:

    __layer_field: LayerField
    __unit_field: UnitField | None = field(default=None, init=False)
    # [XXX] This does not include a unit field!
    _trait_fields: dict[str, _BaseTraitField] = field(default_factory=dict, init=False)

    @overload
    def get_trait_field(self, trait: LayerTrait) -> LayerField: ...
    @overload
    def get_trait_field(self, trait: TimeTrait) -> LayerField: ...
    @overload
    def get_trait_field(self, trait: TransformTrait) -> TransformField: ...
    @overload
    def get_trait_field(self, trait: ShaderTrait) -> ShaderField: ...
    @overload
    def get_trait_field(self, trait: TextureTrait) -> TextureField: ...

    @overload
    def get_trait_field(self, trait: VideoTrait) -> VideoField: ...
    @overload
    def get_trait_field(self, trait: AudioTrait) -> AudioField: ...
    @overload
    def get_trait_field(self, trait: ImageTrait) -> ImageField: ...

    @overload
    def get_trait_field(self, trait: TextTrait) -> TextField: ...
    @overload
    def get_trait_field(self, trait: TextStyleTrait) -> TextStyleField: ...

    @overload
    def get_trait_field(self, trait: RectTrait) -> RectField: ...
    @overload
    def get_trait_field(self, trait: CircleTrait) -> CircleField: ...
    @overload
    def get_trait_field(self, trait: TriTrait) -> TriField: ...
    @overload
    def get_trait_field(self, trait: LineTrait) -> LineField: ...

    @overload
    def get_trait_field(self, trait: UnitTrait) -> UnitField: ...

    @overload
    def get_trait_field(self, trait: _BaseShapeTrait) -> _BaseShapeField: ...
    @overload
    def get_trait_field(self, trait: _BaseMediaTrait) -> _BaseMediaField: ...

    def get_trait_field(self, trait: _BaseTrait) -> _BaseTraitField:
        if trait._name in ['layer', 'time']:
            return self.__layer_field
        if trait._name in ['unit']:
            if not self.__unit_field:
                raise Exception('unit field is None')
            return self.__unit_field

        if trait._name.startswith('base_'):
            raise Exception('Called from base traits')

        if trait._name not in self._trait_fields:
            self._trait_fields[trait._name] = create_trait_field(trait._name)

        return self._trait_fields[trait._name]

    def _get_transform_field(self, secret: str) -> TransformField:

        if secret != "DO NOT CALL THIS! THIS IS FOR PRIVATE USAGE ONLY!":
            raise Exception("DO NOT CALL THIS! THIS IS FOR PRIVATE USAGE ONLY!")

        trait_name = 'transform'

        if trait_name not in self._trait_fields:
            self._trait_fields[trait_name] = create_trait_field(trait_name)

        return tp.cast(TransformField, self._trait_fields[trait_name])

    def _set_unit_field(self, unit_field):
        self.__unit_field = unit_field


@dataclass
class LayerHandle:

    _layer_idx: int = field(init=False)
    _priv: _BaseTraitPriv = field(init=False)

    def ref(self) -> LayerRef:
        return LayerRef(self._layer_idx)

    def __enter__(self) -> tp.Self:
        if gctx.get_ctx()._cur_layer_id != -1:
            raise Exception('Nested layer is forbidden')

        layer_idx = register_layer_field(LayerField())
        gctx.get_ctx()._cur_layer_id = layer_idx
        self._layer_idx = layer_idx

        layer_field = gctx.get_ctx().layers[self._layer_idx]
        self._priv = _BaseTraitPriv(layer_field)
        for fld in fields(self):
            if fld.type.endswith('Trait'):
                setattr(self, fld.name, create_trait(fld.name, self._priv))

        return self

    def __exit__(self, *ext: tp.Any):

        layer_field = gctx.get_ctx().layers[self._layer_idx]

        gctx.get_ctx()._cur_layer_id = -1

        for fld in fields(self):
            if fld.type.endswith('Trait'):
                setattr(self, fld.name, None)

        for tr_name, tr_fld in self._priv._trait_fields.items():
            g_trait_fields = getattr(gctx.get_ctx(), "t_" + tr_name + 's')

            new_trait_idx = -1

            if tr_name not in ['shader', 'unit']:
                # [XXX] Try using trait caches
                for idx, g_tr_fld in enumerate(g_trait_fields):
                    if g_tr_fld == tr_fld:
                        new_trait_idx = idx
                        break

            if new_trait_idx == -1:
                g_trait_fields.append(tr_fld)
                new_trait_idx = len(g_trait_fields) - 1

            setattr(layer_field, "t_" + tr_name, new_trait_idx)

        return False


@dataclass
class VideoHandle(LayerHandle):

    __req_src: str

    t_layer: LayerTrait = field(init=False)
    t_transform: TransformTrait = field(init=False)
    t_texture: TextureTrait = field(init=False)
    t_shader: ShaderTrait = field(init=False)
    t_video: VideoTrait = field(init=False)

    def __enter__(self) -> tp.Self:
        super().__enter__()

        self._priv.get_trait_field(self.t_layer)._duration = sec(-1)
        self._priv.get_trait_field(self.t_video).req_src = self.__req_src
        self._priv.get_trait_field(self.t_transform).pos = lcenter()

        return self

    def __exit__(self, *ext: tp.Any):
        video_field = self._priv.get_trait_field(self.t_video)
        self._priv.get_trait_field(self.t_layer)._duration = calc_media_duration(video_field)

        return super().__exit__()


def video(src: str) -> VideoHandle:
    return VideoHandle(src)


@dataclass
class AudioHandle(LayerHandle):

    __req_src: str

    t_layer: LayerTrait = field(init=False)
    t_audio: AudioTrait = field(init=False)

    def __enter__(self) -> tp.Self:
        super().__enter__()

        self._priv.get_trait_field(self.t_layer)._duration = sec(-1)
        self._priv.get_trait_field(self.t_audio).req_src = self.__req_src

        return self

    def __exit__(self, *ext: tp.Any):
        audio_field = self._priv.get_trait_field(self.t_audio)
        self._priv.get_trait_field(self.t_layer)._duration = calc_media_duration(audio_field)
        return super().__exit__()


def audio(src: str) -> AudioHandle:
    return AudioHandle(src)


@dataclass
class ImageHandle(LayerHandle):

    __req_srcs: list[str]

    t_layer: LayerTrait = field(init=False)
    t_time: TimeTrait = field(init=False)
    t_transform: TransformTrait = field(init=False)
    t_texture: TextureTrait = field(init=False)
    t_shader: ShaderTrait = field(init=False)
    t_image: ImageTrait = field(init=False)

    def __enter__(self) -> tp.Self:
        super().__enter__()

        self._priv.get_trait_field(self.t_image).req_srcs = tuple(self.__req_srcs)
        self._priv.get_trait_field(self.t_transform).pos = lcenter()

        return self

    def __exit__(self, *ext: tp.Any):
        return super().__exit__()


def image(srcs: list[str] | str) -> ImageHandle:
    return ImageHandle([srcs] if isinstance(srcs, str) else srcs)


@dataclass
class TextHandle(LayerHandle):

    __req_text: str

    t_layer: LayerTrait = field(init=False)
    t_time: TimeTrait = field(init=False)
    t_transform: TransformTrait = field(init=False)
    t_texture: TextureTrait = field(init=False)
    t_shader: ShaderTrait = field(init=False)
    t_text: TextTrait = field(init=False)
    t_text_style: TextStyleTrait = field(init=False)

    def __enter__(self) -> tp.Self:
        super().__enter__()

        self._priv.get_trait_field(self.t_text).req_text = self.__req_text
        self._priv.get_trait_field(self.t_transform).pos = lcenter()

        return self

    def __exit__(self, *ext: tp.Any):
        return super().__exit__()


def text(text: str) -> TextHandle:
    return TextHandle(text)


@dataclass
class RectHandle(LayerHandle):

    __req_size: tuple[int, int]

    t_layer: LayerTrait = field(init=False)
    t_time: TimeTrait = field(init=False)
    t_transform: TransformTrait = field(init=False)
    t_texture: TextureTrait = field(init=False)
    t_shader: ShaderTrait = field(init=False)
    t_rect: RectTrait = field(init=False)

    def __enter__(self) -> tp.Self:
        super().__enter__()

        self._priv.get_trait_field(self.t_rect).req_size = self.__req_size
        self._priv.get_trait_field(self.t_transform).pos = lcenter()

        return self

    def __exit__(self, *ext: tp.Any):
        return super().__exit__()


def rect(w: int, h: int) -> RectHandle:
    return RectHandle((w, h))


@dataclass
class CircleHandle(LayerHandle):

    __req_size: int | tuple[int, int]

    t_layer: LayerTrait = field(init=False)
    t_time: TimeTrait = field(init=False)
    t_transform: TransformTrait = field(init=False)
    t_texture: TextureTrait = field(init=False)
    t_shader: ShaderTrait = field(init=False)
    t_circle: CircleTrait = field(init=False)

    def __enter__(self) -> tp.Self:
        super().__enter__()

        self._priv.get_trait_field(self.t_circle).req_size = self.__req_size

        if isinstance(self.__req_size, tuple):
            self._priv.get_trait_field(self.t_transform).layer_size = self.__req_size
        else:
            self._priv.get_trait_field(self.t_transform).layer_size = (
                self.__req_size,
                self.__req_size
            )
        self._priv.get_trait_field(self.t_transform).pos = lcenter()

        return self

    def __exit__(self, *ext: tp.Any):
        return super().__exit__()


@overload
def circle(size: int) -> CircleHandle: ...
@overload
def circle(size: tuple[int, int]) -> CircleHandle: ...


def circle(size: int | tuple[int, int]) -> CircleHandle:
    return CircleHandle(size)


@dataclass
class TriHandle(LayerHandle):

    __req_size: int | tuple[int, int, float] | tuple[int, int, float, float]

    t_layer: LayerTrait = field(init=False)
    t_time: TimeTrait = field(init=False)
    t_transform: TransformTrait = field(init=False)
    t_texture: TextureTrait = field(init=False)
    t_shader: ShaderTrait = field(init=False)
    t_tri: TriTrait = field(init=False)

    def __enter__(self) -> tp.Self:
        super().__enter__()

        field = self._priv.get_trait_field(self.t_tri)
        if isinstance(self.__req_size, tuple):
            field.width = self.__req_size[0]
            field.height = self.__req_size[1]
            field.wr = self.__req_size[2]
            if len(self.__req_size) > 3:
                self.__req_size = tp.cast(tuple[int, int, float, float], self.__req_size)
                field.hr = 0 if (self.__req_size[3] < 0 or self.__req_size[3] > 1) else self.__req_size[3]
        else:
            field.width = self.__req_size
            field.height = int(self.__req_size * 0.5 * math.sqrt(3))
            field.wr = 0.5

        self._priv.get_trait_field(self.t_transform).pos = lcenter()

        return self

    def __exit__(self, *ext: tp.Any):
        return super().__exit__()


@overload
def tri(size: int) -> TriHandle: ...
@overload
def tri(size: tuple[int, int, float]) -> TriHandle: ...
@overload
def tri(size: tuple[int, int, float, float]) -> TriHandle: ...


def tri(size: int | tuple[int, int, float] | tuple[int, int, float, float]) -> TriHandle:
    return TriHandle(size)


@dataclass
class LineHandle(LayerHandle):

    __req_size: float

    t_layer: LayerTrait = field(init=False)
    t_time: TimeTrait = field(init=False)
    t_transform: TransformTrait = field(init=False)
    t_texture: TextureTrait = field(init=False)
    t_shader: ShaderTrait = field(init=False)
    t_line: LineTrait = field(init=False)

    def __enter__(self) -> tp.Self:
        super().__enter__()

        self._priv.get_trait_field(self.t_line).req_size = self.__req_size
        self._priv.get_trait_field(self.t_transform).pos = lcenter()
        self._priv.get_trait_field(self.t_transform).layer_size = (ak_lwidth(), ak_lheight())

        return self

    def __exit__(self, *ext: tp.Any):
        return super().__exit__()


def line(size: float) -> LineHandle:
    return LineHandle(size)


@dataclass
class UnitHandle(LayerHandle):

    __frame_ctx: _FrameFnCtxOpaque
    __guard: SpatialFrameGuard | TimelineFrameGuard = field(init=False)
    __layer_fns: tp.Callable[[], None] = field(init=False)

    t_layer: LayerTrait = field(init=False)
    t_transform: TransformTrait = field(init=False)
    t_texture: TextureTrait = field(init=False)
    t_shader: ShaderTrait = field(init=False)
    t_unit: UnitTrait = field(init=False)

    def __enter__(self) -> tp.Self:
        super().__enter__()

        # Prepare and register a unit field
        layer_field = gctx.get_ctx().layers[self._layer_idx]
        assert layer_field.t_unit == -1
        unit_field = UnitField()
        gctx.get_ctx().t_units.append(unit_field)
        new_unit_trait_idx = len(gctx.get_ctx().t_units) - 1
        layer_field.t_unit = new_unit_trait_idx
        self._priv._set_unit_field(unit_field)

        kind, layer_fns = tp.cast(_FrameFnCtx, self.__frame_ctx)
        self.__layer_fns = layer_fns

        if kind not in ['spatial', 'timeline']:
            raise NotImplementedError()

        if kind == 'spatial':
            self.__guard = SpatialFrameGuard(self._layer_idx)
        else:
            self.__guard = TimelineFrameGuard(self._layer_idx)

        self._priv.get_trait_field(self.t_transform).pos = lcenter()
        self._priv.get_trait_field(self.t_transform).layer_size = (ak_lwidth(), ak_lheight())
        self._priv.get_trait_field(self.t_unit).fb_size = (ak_lwidth(), ak_lheight())

        self.__guard.enter()
        return self

    def __exit__(self, *ext: tp.Any):
        before_lid = gctx.get_ctx()._cur_layer_id
        gctx.get_ctx()._cur_layer_id = -1
        self.__layer_fns()
        gctx.get_ctx()._cur_layer_id = before_lid
        self.__guard.exit()
        return super().__exit__()


@overload
def unit(frame_ctx: _FrameFnCtxOpaque[tp.Literal['spatial']]) -> UnitHandle: ...
@overload
def unit(frame_ctx: _FrameFnCtxOpaque[tp.Literal['timeline']]) -> UnitHandle: ...
@overload
def unit(frame_ctx: tp.Never) -> tp.Never: ...


def unit(frame_ctx) -> UnitHandle:
    return UnitHandle(frame_ctx)
