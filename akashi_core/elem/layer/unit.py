# pyright: reportPrivateUsage=false, reportIncompatibleMethodOverride=false
from dataclasses import dataclass, field
import typing as tp

from .base import (
    FittableDurationTrait,
    ShaderField,
    LayerField,
    LayerTrait,
    PositionField,
    PositionTrait,
    TextureField,
    TextureTrait,
)
from .base import register_entry, peek_entry, frag, poly
from akashi_core.pysl import _gl as gl
from akashi_core.time import sec
from akashi_core.probe import get_duration, g_resource_map
from akashi_core.pysl.shader import (
    ShaderCompiler,
    _frag_shader_header,
    _poly_shader_header,
    LEntryFragFn,
    LEntryPolyFn,
    _NamedEntryFragFn,
    _NamedEntryPolyFn,
    _TEntryFnOpaque
)
from akashi_core.elem.context import _GlobalKronContext as gctx
from akashi_core.color import Color as ColorEnum
from akashi_core.color import color_value

from akashi_core.elem.layout import LayoutFn, LayoutInfo, LayoutLayerContext


@dataclass
class UnitUniform:
    texture0: tp.Final[gl.uniform[gl.sampler2D]] = gl._uniform_default()


@dataclass
class UnitFragBuffer(frag, UnitUniform, gl._LayerFragInput):
    ...


@dataclass
class UnitPolyBuffer(poly, UnitUniform, gl._LayerPolyOutput):
    ...


@dataclass
class UnitEntry(TextureField, ShaderField, PositionField, LayerField):

    layer_indices: list[int] = field(default_factory=list, init=False)
    fb_size: tuple[int, int] = field(default=(0, 0), init=False)
    bg_color: str = "#00000000"  # transparent
    _layout_fn: LayoutFn | None = None


_UnitFragFn = LEntryFragFn[UnitFragBuffer] | _TEntryFnOpaque[_NamedEntryFragFn[UnitFragBuffer]]
_UnitPolyFn = LEntryPolyFn[UnitPolyBuffer] | _TEntryFnOpaque[_NamedEntryPolyFn[UnitPolyBuffer]]


@dataclass
class UnitHandle(TextureTrait, FittableDurationTrait, PositionTrait, LayerTrait):

    def __enter__(self) -> 'UnitHandle':
        gctx.get_ctx()._cur_unit_ids.append(self._idx)
        return self

    def __exit__(self, *ext: tp.Any):
        cur_ctx = gctx.get_ctx()
        cur_unit_layer = tp.cast(UnitEntry, cur_ctx.layers[cur_ctx._cur_unit_ids[-1]])

        if isinstance(cur_unit_layer.duration, sec):
            max_to: sec = sec(0)
            for layout_idx, layer_idx in enumerate(cur_unit_layer.layer_indices):

                cur_layer = cur_ctx.layers[layer_idx]

                if isinstance(cur_layer, PositionField) and cur_unit_layer._layout_fn:

                    layout_info = cur_unit_layer._layout_fn(LayoutLayerContext(
                        cur_layer.key,
                        layout_idx,
                        len(cur_unit_layer.layer_indices)
                    ))
                    if layout_info:
                        cur_layer.pos = layout_info.pos
                        cur_layer.z = layout_info.z

                        temp_layer_size = list(layout_info.layer_size)
                        if cur_layer.kind == 'UNIT':
                            aspect_ratio = sec(tp.cast(UnitEntry, cur_layer).fb_size[0], tp.cast(
                                UnitEntry, cur_layer).fb_size[1])
                            if temp_layer_size[0] == -1:
                                temp_layer_size[0] = (sec(temp_layer_size[1]) * aspect_ratio).trunc()
                            if temp_layer_size[1] == -1:
                                temp_layer_size[1] = (sec(temp_layer_size[0]) / aspect_ratio).trunc()

                        cur_layer.layer_size = (temp_layer_size[0], temp_layer_size[1])

                if isinstance(cur_layer.duration, sec):

                    # resolve -1 duration
                    if cur_layer.kind in ["VIDEO", "AUDIO"] and cur_layer.duration == sec(-1):
                        layer_src: str = cur_layer.src  # type: ignore
                        if layer_src in g_resource_map:
                            cur_ctx.layers[layer_idx].duration = g_resource_map[layer_src]
                        else:
                            cur_ctx.layers[layer_idx].duration = get_duration(layer_src)
                            g_resource_map[layer_src] = tp.cast(sec, cur_ctx.layers[layer_idx].duration)

                    layer_to = cur_ctx.layers[layer_idx].atom_offset + tp.cast(sec, cur_ctx.layers[layer_idx].duration)
                    if layer_to > max_to:
                        max_to = layer_to

            cur_unit_layer.duration = max_to

            # # resolve unit fitted layers
            # for at_layer_idx in unit_fitted_layer_indices:
            #     cur_layers[at_layer_idx].duration = cur_atom._duration

        cur_ctx._cur_unit_ids.pop()
        return False

    def layout(self, layout_fn: LayoutFn) -> 'UnitHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, UnitEntry):
            cur_layer._layout_fn = layout_fn
        return self

    def frag(self, *frag_fns: _UnitFragFn, preamble: tuple[str, ...] = tuple()) -> 'UnitHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, UnitEntry):
            cur_layer.frag_shader = ShaderCompiler(frag_fns, UnitFragBuffer, _frag_shader_header, preamble)
        return self

    def poly(self, *poly_fns: _UnitPolyFn, preamble: tuple[str, ...] = tuple()) -> 'UnitHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, UnitEntry):
            cur_layer.poly_shader = ShaderCompiler(poly_fns, UnitPolyBuffer, _poly_shader_header, preamble)
        return self

    def bg_color(self, color: tp.Union[str, 'ColorEnum']) -> 'UnitHandle':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, UnitEntry):
            cur_layer.bg_color = color_value(color)
        return self


class unit(object):

    frag: tp.ClassVar[tp.Type[UnitFragBuffer]] = UnitFragBuffer
    poly: tp.ClassVar[tp.Type[UnitPolyBuffer]] = UnitPolyBuffer

    def __enter__(self) -> 'UnitHandle':
        raise Exception('unreachable path')

    def __exit__(self, *ext: tp.Any):
        raise Exception('unreachable path')

    def __new__(cls, width: int, height: int, key: str = '') -> UnitHandle:
        entry = UnitEntry()
        entry.layer_size = (width, height)
        entry.fb_size = (width, height)
        idx = register_entry(entry, 'UNIT', key)
        return UnitHandle(idx)
