from uuid import uuid4
from typing import NewType

UUID = NewType('UUID', str)


def gen_uuid() -> UUID:
    return UUID(str(uuid4()))
