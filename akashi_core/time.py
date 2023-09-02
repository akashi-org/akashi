# pyright: reportIncompatibleMethodOverride=false
from __future__ import annotations
from typing import Union, Final, Literal
from fractions import Fraction
from decimal import Decimal
from math import trunc
from datetime import datetime, timedelta


def parse_timestr(tstr: str) -> datetime:

    formats = [
        '%H:%M:%S.%f',
        '%H:%M:%S',
        '%Hh',
        '%Hh%Mm',
        '%Hh%Mm%Ss',
        '%Mm',
        '%Mm%Ss',
        '%Ss',
    ]

    for fmt in formats:
        try:
            return datetime.strptime(tstr, fmt)
        except ValueError:
            continue

    raise Exception('parse_timestr() Failed')


_SEC_LIKE = Union['sec', int, float]

root_time_tp = Literal['ak_root_time']
root_time: Final[root_time_tp] = 'ak_root_time'


class sec(Fraction):

    def __new__(cls, num: Union[int, float, Fraction, str], den: Union[int, None] = None) -> sec:
        if isinstance(num, int):
            return super().__new__(cls, num, den)
        elif isinstance(num, float):
            return super().__new__(cls, Decimal(str(num)))
        elif isinstance(num, str):
            dt = parse_timestr(num)
            delta = timedelta(hours=dt.hour, minutes=dt.minute, seconds=dt.second, microseconds=dt.microsecond)
            return super().__new__(cls, Decimal(str(delta.total_seconds())))
        else:
            return super().__new__(cls, num.numerator, num.denominator)

    def to_json(self):
        return {"num": self.numerator, "den": self.denominator}

    def __hash__(self):
        return hash((self.numerator, self.denominator))

    def __repr__(self):
        return f'sec({self.numerator}/{self.denominator}, {float(self)})'

    def __add__(self, other: _SEC_LIKE) -> sec:
        return sec(super().__add__(sec(other)))

    def __radd__(self, other: _SEC_LIKE) -> sec:
        return sec(super().__radd__(sec(other)))

    def __sub__(self, other: _SEC_LIKE) -> sec:
        return sec(super().__sub__(sec(other)))

    def __rsub__(self, other: _SEC_LIKE) -> sec:
        return sec(super().__rsub__(sec(other)))

    def __mul__(self, other: _SEC_LIKE) -> sec:
        return sec(super().__mul__(sec(other)))

    def __rmul__(self, other: _SEC_LIKE) -> sec:
        return sec(super().__rmul__(sec(other)))

    def __truediv__(self, other: _SEC_LIKE) -> sec:
        return sec(super().__truediv__(sec(other)))

    def __rtruediv__(self, other: _SEC_LIKE) -> sec:
        return sec(super().__rtruediv__(sec(other)))

    def __floordiv__(self, other: _SEC_LIKE) -> sec:
        return sec(super().__floordiv__(sec(other)))

    def __rfloordiv__(self, other: _SEC_LIKE) -> sec:
        return sec(super().__rfloordiv__(sec(other)))

    # def __mod__(self, other: _SEC_LIKE) -> sec:
    #     return sec(super().__mod__(sec(other)))

    # def __rmod__(self, other: _SEC_LIKE) -> sec:
    #     return sec(super().__rmod__(sec(other)))

    # def __pow__(self, other: int) -> sec:
    #     return sec(super().__pow__(sec(other)))

    # def __rpow__(self, other: _SEC_LIKE) -> sec:
    #     return sec(super().__rpow__(sec(other)))

    def __pos__(self) -> sec:
        return sec(super().__pos__())

    def __neg__(self) -> sec:
        return sec(super().__neg__())

    def __abs__(self) -> sec:
        return sec(super().__abs__())

    def __eq__(self, other: _SEC_LIKE) -> bool:
        return super().__eq__(sec(other))

    def __ne__(self, other: _SEC_LIKE) -> bool:
        return not(super().__eq__(sec(other)))

    def __lt__(self, other: _SEC_LIKE) -> bool:
        return super().__lt__(sec(other))

    def __gt__(self, other: _SEC_LIKE) -> bool:
        return super().__gt__(sec(other))

    def __le__(self, other: _SEC_LIKE) -> bool:
        return super().__le__(sec(other))

    def __ge__(self, other: _SEC_LIKE) -> bool:
        return super().__ge__(sec(other))

    def trunc(self) -> int:
        return trunc(self)


NOT_FIXED_SEC: Final[sec] = sec(-100)
