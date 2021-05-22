from typing import Tuple, Callable, Union
import sys
import re
import os
from .kron import RootType, SceneType, AtomType
from .config import AKConf


def version_check() -> Tuple[bool, str]:
    if '.pyenv/versions' not in sys.executable:
        return (True, '')

    sys_ver = sys.version.split(' ')[0]
    exec_ver = re.findall(r'versions/([^/]*)/bin', sys.executable)[0]

    if exec_ver != sys_ver:
        msg = f'''Found version mismatch!
--------------------------------------------------------------------------------------------
sys.version: {sys.version.replace(os.linesep, ' ')}
sys.executable: {sys.executable}

If you use pyenv, you can resolve this issue. Please see the documentation for more details.
--------------------------------------------------------------------------------------------'''
        return (False, msg)
    else:
        return (True, '')


def akexport_elem():
    def inner(fn: Callable[[], Union[RootType, SceneType, AtomType]]):
        sys.modules[fn.__module__].__akashi_export_elem_fn = fn  # type: ignore
        return fn
    return inner


def akexport_config():
    def inner(fn: Callable[[], AKConf]):
        sys.modules[fn.__module__].__akashi_export_config_fn = fn  # type: ignore
        return fn
    return inner
