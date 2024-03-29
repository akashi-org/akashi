# pyright: reportPrivateUsage=false
from __future__ import annotations
from typing import TYPE_CHECKING, Optional, Callable, cast, Any, Type
from dataclasses import dataclass, field
from .uuid import gen_uuid, UUID
from akashi_core.config import AKConf, config_parse
from akashi_core.time import sec, NOT_FIXED_SEC
from akashi_core.probe import get_duration, g_resource_map
from akashi_core.color import Color as ColorEnum
from akashi_core.color import color_value
import sys

if TYPE_CHECKING:
    ElemFn = Callable[[], None]
    # ConfFn = Callable[[], AKConf]

    from .layer import (
        LayerField,
        TransformField,
        TextureField,
        ShaderField,
        VideoField,
        AudioField,
        ImageField,
        TextField,
        TextStyleField,
        RectField,
        CircleField,
        TriField,
        LineField
    )
    from .layer.unit import UnitField


@dataclass
class KronContext:
    uuid: UUID
    config: AKConf
    atoms: list['AtomEntry'] = field(default_factory=list, init=False)
    layers: list['LayerField'] = field(default_factory=list, init=False)

    t_transforms: list['TransformField'] = field(default_factory=list, init=False)
    t_textures: list['TextureField'] = field(default_factory=list, init=False)
    t_shaders: list['ShaderField'] = field(default_factory=list, init=False)
    t_videos: list['VideoField'] = field(default_factory=list, init=False)
    t_audios: list['AudioField'] = field(default_factory=list, init=False)
    t_images: list['ImageField'] = field(default_factory=list, init=False)
    t_texts: list['TextField'] = field(default_factory=list, init=False)
    t_text_styles: list['TextStyleField'] = field(default_factory=list, init=False)

    t_rects: list['RectField'] = field(default_factory=list, init=False)
    t_circles: list['CircleField'] = field(default_factory=list, init=False)
    t_tris: list['TriField'] = field(default_factory=list, init=False)
    t_lines: list['LineField'] = field(default_factory=list, init=False)

    t_units: list['UnitField'] = field(default_factory=list, init=False)

    _cur_unit_ids: list[int] = field(default_factory=list, init=False)
    _cur_layer_id: int = -1

    @staticmethod
    def init(config: AKConf) -> KronContext:
        return KronContext(gen_uuid(), config)


@dataclass
class _GlobalKronContext:

    __ctx: Optional[KronContext] = None

    @classmethod
    def init_ctx(cls, config: AKConf):
        cls.__ctx = KronContext.init(config)

    @classmethod
    def get_ctx(cls) -> KronContext:
        if not cls.__ctx:
            raise Exception('KronContext is null')
        else:
            return cls.__ctx

    @classmethod
    def flush_ctx(cls):
        cls.__ctx = None


@dataclass
class AtomEntry:

    uuid: UUID
    layer_indices: list[int] = field(default_factory=list, init=False)
    bg_color: str = "#000000"  # "#rrggbb"
    _duration: sec = field(default_factory=lambda: sec(0), init=False)


@dataclass
class AtomHandle:

    _atom_idx: int

    def __enter__(self) -> 'AtomHandle':
        return self

    def __exit__(self, *ext: Any):

        cur_atom = _GlobalKronContext.get_ctx().atoms[self._atom_idx]
        cur_layers = _GlobalKronContext.get_ctx().layers
        max_to: sec = sec(0)
        atom_fitted_layer_indices: list[int] = []
        for layer_idx in cur_atom.layer_indices:
            cur_layer = cur_layers[layer_idx]
            if cur_layer.slice_offset == NOT_FIXED_SEC:
                cur_layer.slice_offset = cur_layer.frame_offset

            if isinstance(cur_layer._duration, sec):
                layer_to = cur_layer.slice_offset + cur_layer._duration
                if layer_to > max_to:
                    max_to = layer_to
            else:
                atom_fitted_layer_indices.append(layer_idx)

        cur_atom._duration = max_to

        # resolve atom fitted layers
        for at_layer_idx in atom_fitted_layer_indices:
            cur_layers[at_layer_idx]._duration = cur_atom._duration

        return False

    def bg_color(self, color: str | 'ColorEnum') -> 'AtomHandle':
        cur_atom = _GlobalKronContext.get_ctx().atoms[self._atom_idx]
        cur_atom.bg_color = color_value(color)
        return self


ATOM_INDEX = 0  # temporary


class root:

    @classmethod
    def bg_color(cls, color: str | 'ColorEnum') -> Type['root']:
        cur_atom = _GlobalKronContext.get_ctx().atoms[ATOM_INDEX]
        cur_atom.bg_color = color_value(color)
        return cls


def _root() -> AtomHandle:

    uuid = gen_uuid()
    _atom = AtomEntry(uuid)
    _GlobalKronContext.get_ctx().atoms.append(_atom)
    atom_idx = len(_GlobalKronContext.get_ctx().atoms) - 1

    return AtomHandle(atom_idx)


class _ElemFnOpaque:
    ...


def entry() -> Callable[['ElemFn'], Callable[[_ElemFnOpaque], KronContext]]:
    def _entry(fn: ElemFn):
        def inner(config_path: _ElemFnOpaque) -> KronContext:
            # NB: We assume _ElemFnOpaque to be str
            if not isinstance(config_path, str):
                raise Exception('')
            config = config_parse(cast(str, config_path))
            _GlobalKronContext.flush_ctx()
            _GlobalKronContext.init_ctx(config)
            with _root():
                fn()
            return _GlobalKronContext.get_ctx()
        sys.modules[fn.__module__].__akashi_export_elem_fn = inner  # type: ignore
        return inner
    return _entry


def cur_config() -> AKConf:
    return _GlobalKronContext.get_ctx().config


def _cur_global_size() -> tuple[int, int]:
    return _GlobalKronContext.get_ctx().config.video.resolution


def _cur_local_size() -> tuple[int, int]:
    cur_ctx = _GlobalKronContext.get_ctx()
    unit_ids = cur_ctx._cur_unit_ids
    if len(unit_ids) == 0:
        return cur_ctx.config.video.resolution
    else:
        cur_unit_layer = cur_ctx.layers[unit_ids[-1]]
        assert cur_unit_layer.t_unit != -1
        return cur_ctx.t_units[cur_unit_layer.t_unit].fb_size


def width() -> int:
    return _cur_global_size()[0]


def hwidth() -> int:
    return width() // 2


def height() -> int:
    return _cur_global_size()[1]


def hheight() -> int:
    return height() // 2


def center() -> tuple[int, int]:
    return (width() // 2, height() // 2)


def lwidth() -> int:
    return _cur_local_size()[0]


def lhwidth() -> int:
    return lwidth() // 2


def lheight() -> int:
    return _cur_local_size()[1]


def lhheight() -> int:
    return lheight() // 2


def lcenter() -> tuple[int, int]:
    return (lwidth() // 2, lheight() // 2)
