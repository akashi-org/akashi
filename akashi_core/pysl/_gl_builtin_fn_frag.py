# pyright: reportPrivateUsage=false
from __future__ import annotations

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from akashi_core.pysl._gl_vec import _TGenType

''' 
    8.12 Fragment Processing Functions
'''


def dFdx(x: _TGenType) -> _TGenType: ...


def dFdy(x: _TGenType) -> _TGenType: ...


def fwidth(x: _TGenType) -> _TGenType: ...

# [TODO] Interpolation Functions
