from typing import TypeVar, Callable, Any

T = TypeVar('T')


def begin(*expr: ..., ret: T) -> T:
    return ret


def cond(pred: bool, true_fn: Callable[[], Any], false_fn: Callable[[], Any] = lambda: ...) -> None:
    if pred:
        true_fn()
    else:
        false_fn()
