from __future__ import annotations
from typing import overload
from dataclasses import dataclass

from os import path
import typing as tp
from typing_extensions import assert_type
import itertools
from fractions import Fraction as rat

'''
def min_bar(star_num: int) -> int:
    i = 0
    while True:
        if i > 100:
            raise Exception('')
        if rat(star_num, (1 + i)) <= rat(4, 1):
            return i
        i += 1


def bar_positions(star_num: int, bar_num: int) -> list[tuple[int, ...]]:
    return [c for c in itertools.combinations(range(star_num - 1), bar_num)]


def params_pattern(star_num: int, bar_pos: tuple[int, ...]) -> list[int]:
    base = 0
    rstar = star_num
    res = []
    for p in bar_pos:
        d = (p + 1) - base
        if rstar >= d:
            res.append(d)
            rstar -= d
            base = p + 1
        else:
            res.append(rstar)
            return res

    return res + [rstar]
'''


def gen_mats():

    res = []

    for M in [2, 3, 4]:
        for L in [2, 3, 4]:

            seq_tp = f"gvec{L}"
            mul_vec_tp = f"gvec{M}"
            rmul_vec_tp = f"gvec{L}"

            mul_ovs = []
            rmul_ovs = []
            for i in [2, 3, 4]:
                mul_ovs += [
                    f"    @overload",
                    f"    def __mul__(self, other: gmat{i}x{M}[_TNumber]) -> gmat{i}x{L}[_TNumber]: ..."
                ]
                rmul_ovs += [
                    f"    @overload",
                    f"    def __rmul__(self, other: gmat{L}x{i}[_TNumber]) -> gmat{M}x{i}[_TNumber]: ...",
                ]

            res += [
                '@dataclass',
                f'class gmat{M}x{L}(tp.Generic[_TNumber]):',
                f"    @overload",
                f"    def __init__(self, arg: _MItems, *args: _MItems): ...",
                f"    @overload",
                f"    def __init__(self, arg: gmat{M}x{L}, *args: _Never): ...",
                f"    def __init__(self, arg: tp.Any, *args: tp.Any): ...",
                f"    def __getitem__(self, index: int) -> '{seq_tp}'[_TNumber]: ...",
                f"    def __setitem__(self, index: int, value: '{seq_tp}'[_TNumber]): ...",
                f"    def length(self) -> int: ...",
                f"    def __add__(self, other: Self) -> Self: ...",
                f"    def __sub__(self, other: Self) -> Self: ...",
                *mul_ovs,
                f"    @overload",
                f"    def __mul__(self, other: '{mul_vec_tp}'[_TNumber]) -> '{mul_vec_tp}'[_TNumber]: ...",
                f"    def __mul__(self, other: tp.Any) -> tp.Any: ...",
                *rmul_ovs,
                f"    @overload",
                f"    def __rmul__(self, other: '{rmul_vec_tp}'[_TNumber]) -> '{rmul_vec_tp}'[_TNumber]: ...",
                f"    def __rmul__(self, other: tp.Any) -> tp.Any: ...",
                f"    def __truediv__(self, other: Self) -> Self: ..."
            ]

    return res


def gen_headers() -> list[str]:

    return [
        'from __future__ import annotations',
        'from dataclasses import dataclass',
        'import typing as tp',
        'from typing import overload',
        'from typing_extensions import Self',
        'from ._gl_common import double, uint',
        'if tp.TYPE_CHECKING:',
        '    from ._gl_vec import gvec2, gvec3, gvec4',
        "_TNumber = tp.TypeVar('_TNumber', float, double)",
        "_UItem = int | float | uint | double | bool",
        "_MItems: tp.TypeAlias = _UItem | 'gvec2' | 'gvec3' | 'gvec4'",
        '_Never = tp.NoReturn',
    ]


def gen_aliases() -> list[str]:

    res = []

    for M in [2, 3, 4]:
        for L in [2, 3, 4]:
            res += [
                f"mat{M}x{L} = gmat{M}x{L}[float]",
                f"dmat{M}x{L} = gmat{M}x{L}[double]"
            ]
            if M == L:
                res += [
                    f"mat{M} = gmat{M}x{L}[float]",
                    f"dmat{M} = gmat{M}x{L}[double]"
                ]

    return res


def main():

    outpath = path.normpath(path.join(path.dirname(path.abspath(__file__)), './_gl_mat.py'))

    src = '\n'.join(gen_headers() + gen_mats() + gen_aliases())

    with open(outpath, 'w') as f:
        f.write(src)


if __name__ == '__main__':
    main()
