from __future__ import annotations
from dataclasses import dataclass
from typing import Type, Generic, Literal, Any, TypeVar

_T = TypeVar('_T')


@dataclass
class expr(Generic[_T]):

    v: _T

    def __rshift__(self, other: 'expr[Any]') -> 'expr[Any]':
        return other


@dataclass
class let(expr[_T]):
    ...


_ASSIGN_OP = Literal['=', '+=', '-=', '*=', '/=', '%=', '<<=', '>>=', '|=', '^=', '&=']


@dataclass
class assign(Generic[_T]):

    v: _T

    def eq(self, other: '_T') -> 'expr[_T]':
        return expr(other)

    def op(self, _op: _ASSIGN_OP, other: '_T') -> 'expr[_T]':
        return expr(other)
