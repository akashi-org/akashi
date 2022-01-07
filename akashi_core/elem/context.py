from __future__ import annotations
from typing import TYPE_CHECKING, Optional, Callable
from dataclasses import dataclass, field
from .uuid import gen_uuid, UUID
from akashi_core.config import AKConf
import sys

if TYPE_CHECKING:
    from .atom import AtomEntry
    from .layer.base import LayerField
    ElemFn = Callable[[], None]
    # ConfFn = Callable[[], AKConf]


@dataclass
class KronContext:
    uuid: UUID
    config: AKConf
    atoms: list['AtomEntry'] = field(default_factory=list, init=False)
    layers: list['LayerField'] = field(default_factory=list, init=False)

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


def entry(config: AKConf) -> Callable[['ElemFn'], Callable[[], KronContext]]:
    # [TODO] impl for a case with no config passed (try to load if from home dir?)
    def _entry(fn: ElemFn):
        def inner() -> KronContext:
            _GlobalKronContext.flush_ctx()
            _GlobalKronContext.init_ctx(config)
            fn()
            return _GlobalKronContext.get_ctx()
        sys.modules[fn.__module__].__akashi_export_elem_fn = inner  # type: ignore
        return inner
    return _entry


def cur_config() -> AKConf:
    return _GlobalKronContext.get_ctx().config


def width() -> int:
    return _GlobalKronContext.get_ctx().config.video.resolution[0]


def height() -> int:
    return _GlobalKronContext.get_ctx().config.video.resolution[1]


def center() -> tuple[int, int]:
    return (width() // 2, height() // 2)
