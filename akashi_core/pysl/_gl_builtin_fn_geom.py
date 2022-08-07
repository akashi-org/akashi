# pyright: reportPrivateUsage=false
from __future__ import annotations


''' 
    8.14 Geometry Shader Functions
'''


def EmitStreamVertex(stream: int) -> None: ...


def EndStreamPrimitive(stream: int) -> None: ...


def EmitVertex() -> None: ...


def EndPrimitive() -> None: ...
