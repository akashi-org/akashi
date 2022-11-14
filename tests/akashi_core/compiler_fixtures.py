from akashi_core import ak, gl


@gl.lib('any')
def boost(v: int) -> int:
    return v * 1000


@gl.lib('any')
def boost_add(a: int, b: int) -> int:
    return a + boost(b)


OUTER_VALUE = 12


@gl.lib('any')
def outer_func1(a: int, b: int) -> int:
    return a + gl.outer(OUTER_VALUE)
