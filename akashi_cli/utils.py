from typing import Final
from os import path
import os


def from_relpath(bpath: str, tpath: str) -> str:
    return path.normpath(path.join(path.dirname(path.abspath(bpath)), tpath))


BIN_PATH: Final[str] = (
    from_relpath(__file__, './akashi_renderer')
    if 'AK_BINPATH' not in os.environ.keys()
    else os.environ['AK_BINPATH']
)

ENCODER_BIN_PATH: Final[str] = (
    from_relpath(__file__, './akashi_encoder')
    if 'AK_ENCODER_BINPATH' not in os.environ.keys()
    else os.environ['AK_ENCODER_BINPATH']
)

KERNEL_BIN_PATH: Final[str] = (
    from_relpath(__file__, './akashi_kernel')
    if 'AK_KERNEL_BINPATH' not in os.environ.keys()
    else os.environ['AK_KERNEL_BINPATH']
)

LIBRARY_PATH: Final[str] = from_relpath(__file__, './lib')
