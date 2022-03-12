# pyright: reportPrivateUsage=false
from __future__ import annotations
from typing import TYPE_CHECKING, Optional, Callable, cast
from dataclasses import dataclass, field
from .uuid import gen_uuid, UUID
from akashi_core.config import AKConf, config_parse
import sys

if TYPE_CHECKING:
    from .atom import AtomEntry
    from .layer.base import LayerField
    from .layer.unit import UnitEntry
    ElemFn = Callable[[], None]
    # ConfFn = Callable[[], AKConf]


@dataclass
class KronContext:
    uuid: UUID
    config: AKConf
    atoms: list['AtomEntry'] = field(default_factory=list, init=False)
    layers: list['LayerField'] = field(default_factory=list, init=False)

    _cur_unit_ids: list[int] = field(default_factory=list, init=False)

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
        return cast('UnitEntry', cur_ctx.layers[unit_ids[-1]]).fb_size


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
