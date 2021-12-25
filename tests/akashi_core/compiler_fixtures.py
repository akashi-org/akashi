from akashi_core import ak, gl


@gl.fn('any')
def boost(v: int) -> int:
    return v * 1000


@gl.fn('any')
def boost_add(a: int, b: int) -> int:
    return a + boost(b)
