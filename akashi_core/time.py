# pyright: reportIncompatibleMethodOverride=false
from __future__ import annotations
from typing import Union
from fractions import Fraction
from decimal import Decimal
from math import trunc


class Second(Fraction):

    def __new__(cls, num: Union[int, float, Fraction], den: Union[int, None] = None) -> Second:
        if isinstance(num, int):
            return super().__new__(cls, num, den)
        elif isinstance(num, float):
            return super().__new__(cls, Decimal(str(num)))
        else:
            return super().__new__(cls, num.numerator, num.denominator)

    def to_json(self):
        return {"num": self.numerator, "den": self.denominator}

    def __repr__(self):
        return f'Second({self.numerator}/{self.denominator}, {float(self)})'

    def __add__(self, other: Second) -> Second:
        return Second(super().__add__(other))  # type: ignore

    def __radd__(self, other: Second) -> Second:
        return Second(super().__radd__(other))  # type: ignore

    def __sub__(self, other: Second) -> Second:
        return Second(super().__sub__(other))  # type: ignore

    def __rsub__(self, other: Second) -> Second:
        return Second(super().__rsub__(other))  # type: ignore

    def __mul__(self, other: Second) -> Second:
        return Second(super().__mul__(other))  # type: ignore

    def __rmul__(self, other: Second) -> Second:
        return Second(super().__rmul__(other))  # type: ignore

    def __truediv__(self, other: Second) -> Second:
        return Second(super().__truediv__(other))  # type: ignore

    def __rtruediv__(self, other: Second) -> Second:
        return Second(super().__rtruediv__(other))  # type: ignore

    def __floordiv__(self, other: Second) -> Second:
        return Second(super().__floordiv__(other))  # type: ignore

    def __rfloordiv__(self, other: Second) -> Second:
        return Second(super().__rfloordiv__(other))  # type: ignore

    def __mod__(self, other: Second) -> Second:
        return Second(super().__mod__(other))  # type: ignore

    def __rmod__(self, other: Second) -> Second:
        return Second(super().__rmod__(other))  # type: ignore

    def __divmod__(self, other: Second) -> Second:
        return Second(super().__divmod__(other))  # type: ignore

    def __rdivmod__(self, other: Second) -> Second:
        return Second(super().__rdivmod__(other))  # type: ignore

    def __pow__(self, other: Second) -> Second:
        return Second(super().__pow__(other))  # type: ignore

    def __rpow__(self, other: Second) -> Second:
        return Second(super().__rpow__(other))  # type: ignore

    # [TODO] Even with the settings below, comparison with other types like int still can be possible.

    def __eq__(self, other: Second) -> bool:
        return super().__eq__(other)

    def __ne__(self, other: Second) -> bool:
        return not(super().__eq__(other))

    def __lt__(self, other: Second) -> bool:
        return super().__lt__(other)

    def __gt__(self, other: Second) -> bool:
        return super().__gt__(other)

    def __le__(self, other: Second) -> bool:
        return super().__le__(other)

    def __ge__(self, other: Second) -> bool:
        return super().__ge__(other)

    def trunc(self) -> int:
        return trunc(self)
