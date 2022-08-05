from __future__ import annotations

import itertools
from os import path
import typing as tp


def _indent(lines: list[str], pad: int = 4) -> list[str]:
    return [' ' * pad + l for l in lines]


def _nodup_letters(letter: str) -> list[str]:

    res = []
    for i in range(1, len(letter) + 1):
        res += [''.join(l) for l in itertools.permutations(letter, i)]
    return res


def _odup_letters(letter: str, ndps: list[str]) -> list[str]:

    res = []
    for i in range(1, 4 + 1):
        res += [''.join(l) for l in itertools.product(letter, repeat=i) if ''.join(l) not in ndps]
    return res


def _gen_prop(dim: int) -> list[str]:

    headers = [
        '@dataclass',
        f'class _{dim}d_prop(tp.Generic[_TNumber]):',
        ''
    ]

    bodies = []

    for lsets in ['xyzw', 'rgba', 'stpq']:
        ndps = _nodup_letters(lsets[0:dim])
        for ndp in ndps:
            tp = f'gvec{len(ndp)}[_TNumber]' if len(ndp) > 1 else '_TNumber'
            bodies += [
                '@property',
                f'def {ndp}(self) -> {tp}: ...',
                f'@{ndp}.setter',
                f'def {ndp}(self, value: {tp}): ...'
            ]
        for odp in _odup_letters(lsets[0:dim], ndps):
            tp = f'gvec{len(odp)}[_TNumber]' if len(odp) > 1 else '_TNumber'
            bodies += [
                '@property',
                f'def {odp}(self) -> {tp}: ...'
            ]

    return headers + _indent(bodies)


def gen_props() -> list[str]:

    res = []
    for dim in [2, 3, 4]:
        res += _gen_prop(dim)

    return res


def gen_op() -> list[str]:

    return [
        '@dataclass',
        'class _vec_op(tp.Generic[_TNumber]):',
        '    @overload',
        '    def __add__(self, other: _TNumber) -> Self: ...',
        '    @overload',
        '    def __add__(self, other: Self) -> Self: ...',
        '    def __add__(self, other) -> Self: ...',
        '    @overload',
        '    def __sub__(self, other: _TNumber) -> Self: ...',
        '    @overload',
        '    def __sub__(self, other: Self) -> Self: ...',
        '    def __sub__(self, other) -> Self: ...',
        '    @overload',
        '    def __mul__(self, other: _TNumber) -> Self: ...',
        '    @overload',
        '    def __mul__(self, other: Self) -> Self: ...',
        '    def __mul__(self, other) -> Self: ...',
        '    @overload',
        '    def __truediv__(self, other: _TNumber) -> Self: ...',
        '    @overload',
        '    def __truediv__(self, other: Self) -> Self: ...',
        '    def __truediv__(self, other) -> Self: ...'
    ]


def gen_seq() -> list[str]:

    return [
        '@dataclass',
        'class _vec_seq(tp.Generic[_TNumber]):',
        '    def __getitem__(self, index: int) -> _TNumber: ...',
        '    def __setitem__(self, index: int, value: _TNumber): ...',
        '    def length(self) -> int: ...',
    ]


def gen_vecs() -> list[str]:

    res = []

    res += [
        '@dataclass',
        'class gvec2(_2d_prop[_TNumber], _vec_seq[_TNumber], _vec_op[_TNumber], tp.Generic[_TNumber]):',
        '    @overload',
        '    def __init__(self, p1: _TNumber, p2: _TNumber): ...',
        '    @overload',
        '    def __init__(self, p1: gvec2, p2: _Never = _NV): ...',
        '    @overload',
        '    def __init__(self, p1: _TNumber, p2: _Never = _NV): ...',
        '    def __init__(self, p1: tp.Any, p2: tp.Any = None): ...'
    ]

    res += [
        '@dataclass',
        'class gvec3(_3d_prop[_TNumber], _vec_seq[_TNumber], _vec_op[_TNumber], tp.Generic[_TNumber]):',
        '    @overload',
        '    def __init__(self, p1: _TNumber, p2: _TNumber, p3: _TNumber): ...',
        '    @overload',
        '    def __init__(self, p1: gvec2, p2: _TNumber, p3: _Never = _NV): ...',
        '    @overload',
        '    def __init__(self, p1: _TNumber, p2: gvec2, p3: _Never = _NV): ...',
        '    @overload',
        '    def __init__(self, p1: _TNumber, p2: _Never = _NV, p3: _Never = _NV): ...',
        '    @overload',
        '    def __init__(self, p1: gvec3, p2: _Never = _NV, p3: _Never = _NV): ...',
        '    def __init__(self, p1: tp.Any, p2: tp.Any = None, p3: tp.Any = None): ...'
    ]

    res += [
        '@dataclass',
        'class gvec4(_4d_prop[_TNumber], _vec_seq[_TNumber], _vec_op[_TNumber], tp.Generic[_TNumber]):',
        '    @overload',
        '    def __init__(self, p1: _TNumber, p2: _TNumber, p3: _TNumber, p4: _TNumber): ...',
        '    @overload',
        '    def __init__(self, p1: gvec3, p2: _TNumber, p3: _Never = _NV, p4: _Never = _NV): ...',
        '    @overload',
        '    def __init__(self, p1: _TNumber, p2: gvec3, p3: _Never = _NV, p4: _Never = _NV): ...',
        '    @overload',
        '    def __init__(self, p1: gvec2, p2: _TNumber, p3: _TNumber, p4: _Never = _NV): ...',
        '    @overload',
        '    def __init__(self, p1: _TNumber, p2: gvec2, p3: _TNumber, p4: _Never = _NV): ...',
        '    @overload',
        '    def __init__(self, p1: _TNumber, p2: _TNumber, p3: gvec2, p4: _Never = _NV): ...',
        '    @overload',
        '    def __init__(self, p1: _TNumber, p2: _Never = _NV, p3: _Never = _NV, p4: _Never = _NV): ...',
        '    @overload',
        '    def __init__(self, p1: gvec4, p2: _Never = _NV, p3: _Never = _NV, p4: _Never = _NV): ...',
        '    def __init__(self, p1: tp.Any, p2: tp.Any = None, p3: tp.Any = None, p4: tp.Any = None): ...'
    ]

    return res


def gen_headers() -> list[str]:

    return [
        'from __future__ import annotations',
        'from dataclasses import dataclass',
        'import typing as tp',
        'from typing import overload',
        'from typing_extensions import Self',
        "_TNumber = tp.TypeVar('_TNumber', int, float)",
        '_Never = tp.NoReturn',
        "_NV: tp.Any = None"
    ]


def gen_alias() -> list[str]:

    return [
        'vec4 = gvec4[float]',
        'ivec4 = gvec4[int]',
        'uvec4 = gvec4[uint]',
        'vec3 = gvec3[float]',
        'ivec3 = gvec3[int]',
        'uvec3 = gvec3[uint]',
        'vec2 = gvec2[float]',
        'ivec2 = gvec2[int]',
        'uvec2 = gvec2[uint]'
    ]


def __quickcheck():

    from _gl_vec import gvec2, gvec3, gvec4
    # from _gl_common import uint

    vec4 = gvec4[float]
    ivec4 = gvec4[int]
    # uvec4 = gvec4[uint]
    bvec4 = gvec4[bool]

    vec3 = gvec3[float]
    ivec3 = gvec3[int]
    # uvec3 = gvec3[uint]

    vec2 = gvec2[float]
    ivec2 = gvec2[int]
    # uvec2 = gvec2[uint]

    vec2(1).y = 12


def main():

    outpath = path.normpath(path.join(path.dirname(path.abspath(__file__)), './_gl_vec.py'))

    src = '\n'.join(gen_headers() + gen_seq() + gen_op() + gen_props() + gen_vecs())

    with open(outpath, 'w') as f:
        f.write(src)


if __name__ == '__main__':
    main()
