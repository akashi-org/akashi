from __future__ import annotations
from dataclasses import dataclass
from typing import Type, Generic, TypeVar

_T = TypeVar('_T')


@dataclass
class expr(Generic[_T]):

    v: _T

    def tp(self, _tp: Type[_T]) -> 'expr[_T]':
        return self

    # [XXX] concats multiple exprs
    def __or__(self, other: 'expr') -> 'expr':
        return other

    # [XXX] substitutes for assignment(=)
    def __lshift__(self, other: 'expr[_T]') -> 'expr[_T]':
        return self
