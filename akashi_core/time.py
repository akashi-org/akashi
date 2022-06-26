# pyright: reportIncompatibleMethodOverride=false
from __future__ import annotations
from typing import Union, Final
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

    def __repr__(self):
        return f'sec({self.numerator}/{self.denominator}, {float(self)})'

    def __add__(self, other: sec) -> sec:
        return sec(super().__add__(other))  # type: ignore

    def __radd__(self, other: sec) -> sec:
        return sec(super().__radd__(other))  # type: ignore

    def __sub__(self, other: sec) -> sec:
        return sec(super().__sub__(other))  # type: ignore

    def __rsub__(self, other: sec) -> sec:
        return sec(super().__rsub__(other))  # type: ignore

    def __mul__(self, other: sec) -> sec:
        return sec(super().__mul__(other))  # type: ignore

    def __rmul__(self, other: sec) -> sec:
        return sec(super().__rmul__(other))  # type: ignore

    def __truediv__(self, other: sec) -> sec:
        return sec(super().__truediv__(other))  # type: ignore

    def __rtruediv__(self, other: sec) -> sec:
        return sec(super().__rtruediv__(other))  # type: ignore

    def __floordiv__(self, other: sec) -> sec:
        return sec(super().__floordiv__(other))  # type: ignore

    def __rfloordiv__(self, other: sec) -> sec:
        return sec(super().__rfloordiv__(other))  # type: ignore

    def __mod__(self, other: sec) -> sec:
        return sec(super().__mod__(other))  # type: ignore

    def __rmod__(self, other: sec) -> sec:
        return sec(super().__rmod__(other))  # type: ignore

    def __divmod__(self, other: sec) -> sec:
        return sec(super().__divmod__(other))  # type: ignore

    def __rdivmod__(self, other: sec) -> sec:
        return sec(super().__rdivmod__(other))  # type: ignore

    def __pow__(self, other: sec) -> sec:
        return sec(super().__pow__(other))  # type: ignore

    def __rpow__(self, other: sec) -> sec:
        return sec(super().__rpow__(other))  # type: ignore

    # [TODO] Even with the settings below, comparison with other types like int still can be possible.

    def __eq__(self, other: sec) -> bool:
        return super().__eq__(other)

    def __ne__(self, other: sec) -> bool:
        return not(super().__eq__(other))

    def __lt__(self, other: sec) -> bool:
        return super().__lt__(other)

    def __gt__(self, other: sec) -> bool:
        return super().__gt__(other)

    def __le__(self, other: sec) -> bool:
        return super().__le__(other)

    def __ge__(self, other: sec) -> bool:
        return super().__ge__(other)

    def trunc(self) -> int:
        return trunc(self)


NOT_FIXED_SEC: Final[sec] = sec(-100)
