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
    ShaderTrait,
    LayerField,
    LayerTrait,
    LayerTimeTrait,
    CropField,
    CropTrait
)
from .base import peek_entry, register_entry, LayerRef
from akashi_core.pysl import _gl as gl

from akashi_core.elem.context import lcenter


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
class ImageTrait(LayerTrait, LayerTimeTrait):

    transform: TransformTrait = field(init=False)
    crop: CropTrait = field(init=False)
    tex: TextureTrait = field(init=False)
    shader: ShaderTrait = field(init=False)

    def __post_init__(self):
        self.transform = TransformTrait(self._idx)
        self.crop = CropTrait(self._idx)
        self.tex = TextureTrait(self._idx)
        self.shader = ShaderTrait(self._idx)


ImageTraitFn = tp.Callable[[ImageTrait], tp.Any]


def image(src: str | list[str], *trait_fns: ImageTraitFn) -> LayerRef:

    entry = ImageEntry([src] if isinstance(src, str) else src)
    idx = register_entry(entry, 'IMAGE', '')
    t = ImageTrait(idx)
    t.transform.pos(*lcenter())
    [tfn(t) for tfn in trait_fns]
    return LayerRef(idx)
