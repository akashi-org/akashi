from __future__ import annotations
from typing import TYPE_CHECKING, Optional, Callable
from dataclasses import dataclass, field
from .uuid import gen_uuid, UUID
import sys

if TYPE_CHECKING:
    from .atom import AtomEntry
    from .layer.base import LayerField
    ElemFn = Callable[[], None]
    # ConfFn = Callable[[], AKConf]


@dataclass
class KronContext:
    uuid: UUID
    atoms: list['AtomEntry'] = field(default_factory=list, init=False)
    layers: list['LayerField'] = field(default_factory=list, init=False)

    @staticmethod
    def init() -> KronContext:
        return KronContext(gen_uuid())


@dataclass
class _GlobalKronContext:

    __ctx: Optional[KronContext] = None

    @classmethod
    def get_ctx(cls) -> KronContext:
        if cls.__ctx:
            return cls.__ctx
        else:
            cls.__ctx = KronContext.init()
            return cls.__ctx

    @classmethod
    def flush_ctx(cls):
        cls.__ctx = None


def entry() -> Callable[['ElemFn'], Callable[[], KronContext]]:
    def _entry(fn: ElemFn):
        def inner() -> KronContext:
            _GlobalKronContext.flush_ctx()
            fn()
            return _GlobalKronContext.get_ctx()
        sys.modules[fn.__module__].__akashi_export_elem_fn = inner  # type: ignore
        return inner
    return _entry
