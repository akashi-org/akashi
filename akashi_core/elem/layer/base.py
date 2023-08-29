# pyright: reportPrivateUsage=false
from __future__ import annotations
from dataclasses import dataclass, field
import typing as tp
from typing import runtime_checkable

from akashi_core.elem.context import _GlobalKronContext as gctx
from akashi_core.elem.uuid import UUID, gen_uuid
from akashi_core.time import sec, NOT_FIXED_SEC
from akashi_core.pysl.shader import ShaderCompiler, _frag_shader_header, _poly_shader_header
from akashi_core.pysl.shader import _TEntryFnOpaque, ShaderMemoType


from akashi_core.pysl import _gl

from akashi_core.probe import get_duration

if tp.TYPE_CHECKING:
    from akashi_core.elem.context import root
    from .unit import UnitEntry


LayerKind = tp.Literal['LAYER', 'VIDEO', 'AUDIO', 'TEXT', 'IMAGE', 'UNIT', 'SHAPE', 'FREE']


@dataclass(frozen=True)
class LayerRef:
    value: int


''' Layer Concept '''

_TLayerTrait = tp.TypeVar('_TLayerTrait', bound='LayerTrait')
_TLayerTimeTrait = tp.TypeVar('_TLayerTimeTrait', bound='LayerTimeTrait')


@dataclass
class LayerField:
    uuid: UUID = UUID('')
    atom_uuid: UUID = UUID('')
    kind: LayerKind = 'LAYER'
    key: str = ''
    _duration: sec | 'tp.Type[root]' = field(default_factory=lambda: sec(5))
    slice_offset: sec = field(default_factory=lambda: sec(NOT_FIXED_SEC))
    layer_local_offset: sec = field(default_factory=lambda: sec(0))
    frame_offset: sec = field(default_factory=lambda: sec(0))
    defunct: bool = False


@dataclass
class LayerTrait:

    _idx: int

    def key(self: '_TLayerTrait', value: str) -> '_TLayerTrait':
        if (cur_layer := peek_entry(self._idx)):
            cur_layer.key = value
        return self


@dataclass
class LayerTimeTrait:

    _idx: int

    def duration(self: '_TLayerTimeTrait', duration: sec | float | 'LayerRef' | 'tp.Type[root]') -> '_TLayerTimeTrait':
        if (cur_layer := peek_entry(self._idx)):
            _duration: sec | 'tp.Type[root]'
            if isinstance(duration, LayerRef):
                _duration = gctx.get_ctx().layers[int(duration.value)]._duration
                if not isinstance(_duration, sec):
                    raise Exception('Invalid LayerRef found')
            elif isinstance(duration, (int, float)):
                _duration = sec(duration)
            else:
                _duration = duration

            tp.cast(LayerField, cur_layer)._duration = _duration
        return self

    def offset(self: '_TLayerTimeTrait', offset: sec | float) -> '_TLayerTimeTrait':
        if (cur_layer := peek_entry(self._idx)):
            cur_layer.frame_offset = sec(offset)
        return self


''' Transform Concept '''


@dataclass
class TransformField:
    pos: tuple[int, int] = (0, 0)
    z: float = 0.0
    layer_size: tuple[int, int] = (-1, -1)
    rotation: sec = field(default_factory=lambda: sec(0))


@runtime_checkable
class HasTransformField(tp.Protocol):
    transform: TransformField


@dataclass
class TransformTrait:

    _idx: int

    def pos(self, x: int, y: int) -> 'TransformTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(HasTransformField, cur_layer).transform.pos = (x, y)
        return self

    def z(self, value: float) -> 'TransformTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(HasTransformField, cur_layer).transform.z = value
        return self

    def layer_size(self, width: int, height: int) -> 'TransformTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(HasTransformField, cur_layer).transform.layer_size = (width, height)
        return self

    def rotate(self, degrees: int | float | sec) -> 'TransformTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(HasTransformField, cur_layer).transform.rotation = sec(degrees)
        return self


''' Shader Concept '''


frag = _gl._frag
poly = _gl._poly


@dataclass
class ShaderField:
    frag_shader: tp.Optional[ShaderCompiler] = None
    poly_shader: tp.Optional[ShaderCompiler] = None


@runtime_checkable
class HasShaderField(tp.Protocol):
    shader: ShaderField


_FragFn = _TEntryFnOpaque[tp.Callable[[frag, _gl.frag_color], None]]
_PolyFn = _TEntryFnOpaque[tp.Callable[[poly, _gl.poly_pos], None]]


@dataclass
class ShaderTrait:

    _idx: int

    def frag(self, *frag_fns: _FragFn, preamble: tuple[str, ...] = tuple(), memo: ShaderMemoType = None) -> 'ShaderTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(HasShaderField, cur_layer).shader.frag_shader = ShaderCompiler(
                frag_fns, frag, _frag_shader_header, preamble, memo)
        return self

    def poly(self, *poly_fns: _PolyFn, preamble: tuple[str, ...] = tuple(), memo: ShaderMemoType = None) -> 'ShaderTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(HasShaderField, cur_layer).shader.poly_shader = ShaderCompiler(
                poly_fns, poly, _poly_shader_header, preamble, memo)

        return self


''' Crop Concept '''


@dataclass
class CropField:
    crop_begin: tuple[int, int] = (0, 0)
    crop_end: tuple[int, int] = (0, 0)


@runtime_checkable
class HasCropField(tp.Protocol):
    crop: CropField


@dataclass
class CropTrait:

    _idx: int

    def crop_begin(self, x: int, y: int) -> 'CropTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(HasCropField, cur_layer).crop.crop_begin = (x, y)
        return self

    def crop_end(self, x: int, y: int) -> 'CropTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(HasCropField, cur_layer).crop.crop_end = (x, y)
        return self


''' Texture Concept '''


@dataclass
class TextureField:
    flip_v: bool = False
    flip_h: bool = False


@runtime_checkable
class HasTextureField(tp.Protocol):
    tex: TextureField


@dataclass
class TextureTrait:

    _idx: int

    def flip_v(self, enable_flag: bool = True) -> 'TextureTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(HasTextureField, cur_layer).tex.flip_v = enable_flag
        return self

    def flip_h(self, enable_flag: bool = True) -> 'TextureTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(HasTextureField, cur_layer).tex.flip_h = enable_flag
        return self


''' Media Concept '''


@dataclass
class MediaField:
    src: str
    gain: float = 1.0
    start: sec = field(default_factory=lambda: sec(0))
    end: sec = field(default_factory=lambda: sec(-1))
    _span_cnt: int | None = None
    _span_dur: None | sec | 'tp.Type[root]' = None


@runtime_checkable
class HasMediaField(tp.Protocol):
    media: MediaField


@dataclass
class MediaTrait:

    _idx: int

    def gain(self, gain: float) -> 'MediaTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(HasMediaField, cur_layer).media.gain = gain
        return self

    def range(self, start: sec | float, end: sec | float = -1) -> 'MediaTrait':
        if start < 0:
            raise Exception('Negative start value is prohibited')
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(HasMediaField, cur_layer).media.start = sec(start)
            tp.cast(HasMediaField, cur_layer).media.end = sec(end)
        return self

    def span_cnt(self, count: int) -> 'MediaTrait':
        if (cur_layer := peek_entry(self._idx)):
            tp.cast(HasMediaField, cur_layer).media._span_cnt = count
            tp.cast(HasMediaField, cur_layer).media._span_dur = None
        return self

    def span_dur(self, duration: sec | float | 'LayerRef' | 'tp.Type[root]') -> 'MediaTrait':
        if (cur_layer := peek_entry(self._idx)):
            _duration: sec | 'tp.Type[root]'
            if isinstance(duration, LayerRef):
                _duration = gctx.get_ctx().layers[int(duration.value)]._duration
                if not isinstance(_duration, sec):
                    raise Exception('Invalid LayerRef found')
            elif isinstance(duration, (int, float)):
                _duration = sec(duration)
            else:
                _duration = duration

            tp.cast(HasMediaField, cur_layer).media._span_cnt = None
            tp.cast(HasMediaField, cur_layer).media._span_dur = _duration
        return self


def _calc_media_duration(media: MediaField):
    if media.end == sec(-1):
        media.end = get_duration(media.src)

    media_dur = media.end - media.start
    if media._span_cnt:
        return media_dur * media._span_cnt
    elif media._span_dur:
        return media._span_dur
    else:
        return media_dur


def __is_atom_active(atom_uuid: UUID, raise_exp: bool = True) -> bool:

    cur_atom = gctx.get_ctx().atoms[-1]
    r = atom_uuid == cur_atom.uuid
    if raise_exp and not (r):
        raise Exception('Update for an inactive atom is forbidden')
    else:
        return r


def peek_entry(layer_idx: int) -> tp.Optional[LayerField]:

    cur_layer = gctx.get_ctx().layers[layer_idx]

    if not __is_atom_active(cur_layer.atom_uuid, True):
        return None
    else:
        return cur_layer


def register_entry(entry: LayerField, kind: LayerKind, key: str, append_layer_indices: bool = True) -> int:

    cur_ctx = gctx.get_ctx()
    cur_atom = cur_ctx.atoms[-1]

    # LayerField
    entry.uuid = gen_uuid()
    entry.atom_uuid = cur_atom.uuid
    entry.kind = kind
    entry.key = key

    cur_ctx.layers.append(entry)
    cur_layer_idx = len(cur_ctx.layers) - 1

    if append_layer_indices:
        if len(gctx.get_ctx()._cur_unit_ids) == 0:
            cur_atom.layer_indices.append(cur_layer_idx)
        else:
            cur_unit_layer = tp.cast('UnitEntry', cur_ctx.layers[gctx.get_ctx()._cur_unit_ids[-1]])
            cur_unit_layer.unit.layer_indices.append(cur_layer_idx)

    return cur_layer_idx
