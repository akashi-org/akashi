# pyright: reportPrivateUsage=false
from __future__ import annotations
from dataclasses import dataclass, field
import typing as tp

from .base import (
    TransformField,
    TransformTrait,
    TextureField,
    TextureTrait,
    ShaderField,
    LayerField,
    LayerTrait,
    CropField,
    CropTrait
)
from .base import peek_entry, register_entry, frag, poly, LayerRef
from akashi_core.pysl import _gl as gl
from akashi_core.pysl.shader import ShaderCompiler, _frag_shader_header, _poly_shader_header
from akashi_core.pysl.shader import LEntryFragFn, LEntryPolyFn
from akashi_core.pysl.shader import _NamedEntryFragFn, _NamedEntryPolyFn, _TEntryFnOpaque

from akashi_core.elem.context import lcenter


@dataclass
class ImageUniform:
    texture_arr: tp.Final[gl.uniform[gl.sampler2DArray]] = gl._uniform_default()


@dataclass
class ImageFragBuffer(frag, ImageUniform, gl._LayerFragInput):
    ...


@dataclass
class ImagePolyBuffer(poly, ImageUniform, gl._LayerPolyOutput):
    ...


_ImageFragFn = LEntryFragFn[ImageFragBuffer] | _TEntryFnOpaque[_NamedEntryFragFn[ImageFragBuffer]]
_ImagePolyFn = LEntryPolyFn[ImagePolyBuffer] | _TEntryFnOpaque[_NamedEntryPolyFn[ImagePolyBuffer]]


@dataclass
class ImageLocalField:
    srcs: list[str]


@dataclass
class RequiredParams:
    _req_srcs: list[str]


@dataclass
class ImageEntry(LayerField, RequiredParams):

    image: ImageLocalField = field(init=False)
    transform: TransformField = field(init=False)
    crop: CropField = field(init=False)
    tex: TextureField = field(init=False)
    shader: ShaderField = field(init=False)

    def __post_init__(self):
        self.image = ImageLocalField(self._req_srcs)
        self.transform = TransformField()
        self.crop = CropField()
        self.tex = TextureField()
        self.shader = ShaderField()


@dataclass
class ImageTrait(LayerTrait):

    transform: TransformTrait = field(init=False)
    crop: CropTrait = field(init=False)
    tex: TextureTrait = field(init=False)

    def __post_init__(self):
        self.transform = TransformTrait(self._idx)
        self.crop = CropTrait(self._idx)
        self.tex = TextureTrait(self._idx)

    def frag(self, *frag_fns: _ImageFragFn, preamble: tuple[str, ...] = tuple()) -> 'ImageTrait':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, ImageEntry):
            cur_layer.shader.frag_shader = ShaderCompiler(frag_fns, ImageFragBuffer, _frag_shader_header, preamble)
        return self

    def poly(self, *poly_fns: _ImagePolyFn, preamble: tuple[str, ...] = tuple()) -> 'ImageTrait':
        if (cur_layer := peek_entry(self._idx)) and isinstance(cur_layer, ImageEntry):
            cur_layer.shader.poly_shader = ShaderCompiler(poly_fns, ImagePolyBuffer, _poly_shader_header, preamble)
        return self


image_frag = ImageFragBuffer

image_poly = ImagePolyBuffer

ImageTraitFn = tp.Callable[[ImageTrait], tp.Any]


def image(src: str | list[str], *trait_fns: ImageTraitFn) -> LayerRef:

    entry = ImageEntry([src] if isinstance(src, str) else src)
    idx = register_entry(entry, 'IMAGE', '')
    t = ImageTrait(idx)
    t.transform.pos(*lcenter())
    [tfn(t) for tfn in trait_fns]
    return LayerRef(idx)
