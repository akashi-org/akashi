from uuid import uuid4
from uuid import UUID as _ORIG_UUID
import typing as tp
from random import Random


UUID = tp.NewType('UUID', str)

_rgen = Random()


def _default_gen() -> _ORIG_UUID:
    return uuid4()


def _custom_gen() -> _ORIG_UUID:
    return _ORIG_UUID(int=_rgen.getrandbits(128), version=4)


def gen_uuid() -> UUID:
    return UUID(str(_custom_gen()))
