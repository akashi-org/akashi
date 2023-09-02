# pyright: reportPrivateUsage=false
from __future__ import annotations
from dataclasses import dataclass, field
import typing as tp

from akashi_core.elem.context import _GlobalKronContext as gctx
from akashi_core.elem.uuid import UUID, gen_uuid
from akashi_core.time import sec, NOT_FIXED_SEC, root_time_tp, root_time
from akashi_core.pysl.shader import ShaderCompiler, _frag_shader_header, _poly_shader_header
from akashi_core.pysl.shader import _TEntryFnOpaque, ShaderMemoType
from akashi_core.pysl import _gl

if tp.TYPE_CHECKING:
    from .layer import _BaseTraitPriv


@dataclass(frozen=True)
class LayerRef:
    value: int


''' Layer Concept '''


@dataclass
class _BaseTraitField:
    ...


@dataclass
class _BaseTrait:
    _priv: '_BaseTraitPriv'
    _name: str = "base_"


@dataclass(unsafe_hash=True)
class LayerField(_BaseTraitField):
    uuid: UUID = UUID('')
    atom_uuid: UUID = UUID('')
    key: str = ''
    _duration: sec | root_time_tp = field(default_factory=lambda: sec(5))
    slice_offset: sec = field(default_factory=lambda: sec(NOT_FIXED_SEC))
    layer_local_offset: sec = field(default_factory=lambda: sec(0))
    frame_offset: sec = field(default_factory=lambda: sec(0))
    defunct: bool = False

    t_transform: int = -1
    t_texture: int = -1
    t_shader: int = -1

    t_video: int = -1
    t_audio: int = -1
    t_image: int = -1
    t_text: int = -1
    t_text_style: int = -1

    t_rect: int = -1
    t_circle: int = -1
    t_tri: int = -1
    t_line: int = -1

    t_unit: int = -1


@dataclass
class LayerTrait(_BaseTrait):
    _name: str = "layer"

    def key(self, key: str) -> tp.Self:
        self._priv.get_trait_field(self).key = key
        return self


''' Time Concept '''


@dataclass
class TimeTrait(_BaseTrait):
    _name: str = "time"

    def duration(self, duration: sec | float | 'LayerRef' | root_time_tp) -> tp.Self:
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

        self._priv.get_trait_field(self)._duration = _duration
        return self

    def offset(self, offset: sec | float) -> tp.Self:
        self._priv.get_trait_field(self).frame_offset = sec(offset)
        return self


''' Transform Concept '''


@dataclass(unsafe_hash=True)
class TransformField(_BaseTraitField):
    pos: tuple[int, int] = (0, 0)
    z: float = 0.0
    layer_size: tuple[int, int] = (-1, -1)
    rotation: sec = field(default_factory=lambda: sec(0))


@dataclass
class TransformTrait(_BaseTrait):
    _name: str = "transform"

    def pos(self, x: int, y: int) -> tp.Self:
        self._priv.get_trait_field(self).pos = (x, y)
        return self

    def z(self, value: float) -> tp.Self:
        self._priv.get_trait_field(self).z = value
        return self

    def layer_size(self, width: int, height: int) -> tp.Self:
        self._priv.get_trait_field(self).layer_size = (width, height)
        return self

    def rotate(self, degrees: int | float | sec) -> tp.Self:
        self._priv.get_trait_field(self).rotation = sec(degrees)
        return self


''' Shader Concept '''


frag = _gl._frag
poly = _gl._poly


@dataclass
class ShaderField(_BaseTraitField):
    frag_shader: tp.Optional[ShaderCompiler] = None
    poly_shader: tp.Optional[ShaderCompiler] = None


_FragFn = _TEntryFnOpaque[tp.Callable[[frag, _gl.frag_color], None]]
_PolyFn = _TEntryFnOpaque[tp.Callable[[poly, _gl.poly_pos], None]]


@dataclass
class ShaderTrait(_BaseTrait):

    _name: str = "shader"

    def frag(self, *frag_fns: _FragFn, preamble: tuple[str, ...] = tuple(), memo: ShaderMemoType = None) -> tp.Self:
        self._priv.get_trait_field(self).frag_shader = ShaderCompiler(
            frag_fns, frag, _frag_shader_header, preamble, memo)
        return self

    def poly(self, *poly_fns: _PolyFn, preamble: tuple[str, ...] = tuple(), memo: ShaderMemoType = None) -> tp.Self:
        self._priv.get_trait_field(self).poly_shader = ShaderCompiler(
            poly_fns, poly, _poly_shader_header, preamble, memo)
        return self


''' Texture Concept '''


@dataclass(unsafe_hash=True)
class TextureField(_BaseTraitField):
    flip_v: bool = False
    flip_h: bool = False
    crop_begin: tuple[int, int] = (0, 0)
    crop_end: tuple[int, int] = (0, 0)


@dataclass
class TextureTrait(_BaseTrait):

    _name: str = "texture"

    def flip_v(self, enable_flag: bool = True) -> tp.Self:
        self._priv.get_trait_field(self).flip_v = enable_flag
        return self

    def flip_h(self, enable_flag: bool = True) -> tp.Self:
        self._priv.get_trait_field(self).flip_h = enable_flag
        return self

    # [TODO] impl later
    # def crop_begin(self, x: int, y: int) -> tp.Self:
    #     self._priv.get_trait_field(self).crop_begin = (x, y)
    #     return self

    # def crop_end(self, x: int, y: int) -> tp.Self:
    #     self._priv.get_trait_field(self).crop_end = (x, y)
    #     return self


def register_layer_field(layer_field: LayerField, key: str = '', append_layer_indices: bool = True) -> int:

    cur_ctx = gctx.get_ctx()
    cur_atom = cur_ctx.atoms[-1]

    layer_field.key = key
    layer_field.uuid = gen_uuid()
    layer_field.atom_uuid = cur_atom.uuid

    cur_ctx.layers.append(layer_field)
    cur_layer_idx = len(cur_ctx.layers) - 1

    if append_layer_indices:
        if len(gctx.get_ctx()._cur_unit_ids) == 0:
            cur_atom.layer_indices.append(cur_layer_idx)
        else:
            cur_unit_layer = cur_ctx.layers[gctx.get_ctx()._cur_unit_ids[-1]]
            assert cur_unit_layer.t_unit != -1
            # [XXX] Watch out! Maybe bad things will happen...
            cur_unit_field = cur_ctx.t_units[cur_unit_layer.t_unit]
            cur_unit_field.layer_indices.append(cur_layer_idx)

    return cur_layer_idx


# def __is_atom_active(atom_uuid: UUID, raise_exp: bool = True) -> bool:
#
#     cur_atom = gctx.get_ctx().atoms[-1]
#     r = atom_uuid == cur_atom.uuid
#     if raise_exp and not (r):
#         raise Exception('Update for an inactive atom is forbidden')
#     else:
#         return r
#
#
