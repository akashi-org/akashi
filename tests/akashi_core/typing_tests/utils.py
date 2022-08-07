import typing as tp
from typing import overload
from dataclasses import dataclass
from typing_extensions import LiteralString

_T = tp.TypeVar('_T')
_TMsg = tp.TypeVar('_TMsg', bound=LiteralString)


@dataclass
class TJResult(tp.Generic[_T, _TMsg]):
    ...


TJ_TRUE = TJResult[tp.Literal[True], _TMsg]
TJ_FALSE = TJResult[tp.Literal[False], _TMsg]


@dataclass
class TJ(tp.Generic[_T, _TMsg]):

    actual: _T
    msg: _TMsg = tp.cast(_TMsg, 'NoMsg')

    @overload
    def eq(self, expect: tp.Type[_T]) -> TJ_TRUE: ...

    @overload
    def eq(self, expect: tp.Any) -> TJ_FALSE[_TMsg]: ...

    def eq(self, expect: tp.Any) -> tp.Any: ...

    @overload
    def neq(self, expect: tp.Type[_T]) -> TJ_FALSE[_TMsg]: ...

    @overload
    def neq(self, expect: tp.Any) -> TJ_TRUE: ...

    def neq(self, expect: tp.Any) -> tp.Any: ...
