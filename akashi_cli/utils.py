from typing import Final
from os import path
import os
import sysconfig


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


def libpython_path() -> str:

    libdir = sysconfig.get_config_var("LIBDIR")
    libname = sysconfig.get_config_var("LDLIBRARY")

    if libdir and libname:
        return os.path.join(libdir, libname)
    else:
        raise Exception(f'akashi: error: libpython.so not found: LIBDIR: {libdir}, LDLIBRARY: {libname}')
