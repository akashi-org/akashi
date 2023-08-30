# pyright: reportPrivateUsage=false
from __future__ import annotations
from dataclasses import dataclass, field
import typing as tp
from akashi_core.color import Color as ColorEnum
from akashi_core.color import color_value

from .base import (
    _BaseTrait,
    _BaseTraitField,
)

''' Shape Concept '''

BorderDirection = tp.Literal['inner', 'outer', 'full']


@dataclass(unsafe_hash=True)
class _BaseShapeField(_BaseTraitField):
    fill_color: str = ""  # "#rrggbb" or "#rrggbbaa"
    border_size: float = 0
    border_color: str = ""  # "#rrggbb" or "#rrggbbaa"
    border_direction: BorderDirection = 'inner'
    edge_radius: float = 0


@dataclass
class _BaseShapeTrait(_BaseTrait):

    _name: str = 'base_shape'

    def fill_color(self, color: tp.Union[str, 'ColorEnum']) -> tp.Self:
        self._priv.get_trait_field(self).fill_color = color_value(color)
        return self

    def border_size(self, size: float) -> tp.Self:
        self._priv.get_trait_field(self).border_size = size
        return self

    def border_color(self, color: tp.Union[str, 'ColorEnum']) -> tp.Self:
        self._priv.get_trait_field(self).border_color = color_value(color)
        return self

    def border_direction(self, direction: BorderDirection) -> tp.Self:
        self._priv.get_trait_field(self).border_direction = direction
        return self

    def edge_radius(self, radius: float) -> tp.Self:
        self._priv.get_trait_field(self).edge_radius = radius
        return self


''' Rect Concept '''


@dataclass(unsafe_hash=True)
class RectField(_BaseShapeField):
    req_size: tuple[int, int] = (0, 0)


@dataclass
class RectTrait(_BaseShapeTrait):

    _name: str = 'rect'


''' Circle Concept '''


@dataclass(unsafe_hash=True)
class CircleField(_BaseShapeField):
    req_size: int | tuple[int, int] = 0


@dataclass
class CircleTrait(_BaseShapeTrait):

    _name: str = 'circle'


''' Tri Concept '''


@dataclass(unsafe_hash=True)
class TriField(_BaseShapeField):
    width: int = 0
    height: int = 0
    wr: float = 0.0
    hr: float = 1.0  # range: 0 <= hr <= 1.0


@dataclass
class TriTrait(_BaseShapeTrait):

    _name: str = 'tri'


''' Line Concept '''


LineStyle = tp.Literal['default', 'round-dot', 'square-dot', 'cap']


@dataclass(unsafe_hash=True)
class LineField(_BaseShapeField):
    req_size: float = 0
    req_begin: tuple[int, int] = (0, 0)
    req_end: tuple[int, int] = (0, 0)
    style: LineStyle = 'default'


@dataclass
class LineTrait(_BaseShapeTrait):

    _name: str = 'line'

    def begin(self, x: int, y: int) -> tp.Self:
        self._priv.get_trait_field(self).req_begin = (x, y)
        return self

    def end(self, x: int, y: int) -> tp.Self:
        self._priv.get_trait_field(self).req_end = (x, y)
        return self

    # # [TODO] Impl later
    # def style(self, style: LineStyle) -> tp.Self:
    #     self.__priv.get_trait_field(self).style = style
    #     return self
