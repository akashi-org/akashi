from __future__ import annotations
from dataclasses import dataclass, field
from typing import Any
from .context import _GlobalKronContext as gctx
from .uuid import gen_uuid, UUID


@dataclass
class Atom:

    uuid: UUID
    layer_indices: list[int] = field(default_factory=list, init=False)

    def __enter__(self):
        return self

    def __exit__(self, *ext: Any):
        return False

    @staticmethod
    def begin() -> Atom:

        uuid = gen_uuid()
        _atom = Atom(uuid)
        gctx.get_ctx().atoms.append(_atom)

        return _atom


def atom() -> Atom:

    return Atom.begin()
