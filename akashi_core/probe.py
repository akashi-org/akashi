import ctypes
from os import path
import os
import typing as tp
from dataclasses import dataclass

from akashi_core.time import sec


class _AKFraction(ctypes.Structure):
    _fields_ = [("num", ctypes.c_size_t), ("den", ctypes.c_size_t)]


def _from_relpath(bpath: str, tpath: str) -> str:
    return path.normpath(path.join(path.dirname(path.abspath(bpath)), tpath))


_LIBAKPROBE_PATH: tp.Final[str] = (
    _from_relpath(__file__, './libakprobe.so')
    if 'AK_LIBPROBE_PATH' not in os.environ.keys()
    else os.environ['AK_LIBPROBE_PATH']
)

try:
    ctypes.CDLL('libavformat.so', ctypes.RTLD_GLOBAL)
    ctypes.CDLL('libavutil.so', ctypes.RTLD_GLOBAL)
    _API = ctypes.CDLL(_LIBAKPROBE_PATH)
    _API.akprobe_get_duration.argtypes = (ctypes.POINTER(_AKFraction), ctypes.c_char_p)
    _API.akprobe_get_duration.restype = ctypes.c_int
except Exception as e:
    print(e)
    _API = None


ResourceMapType = dict[str, sec]
g_resource_map: ResourceMapType = {}


def get_duration(url: str) -> sec:

    if not _API:
        raise Exception('API is null')

    duration = _AKFraction()
    _API.akprobe_get_duration(duration, url.encode('utf-8'))

    return sec(duration.num, duration.den)
