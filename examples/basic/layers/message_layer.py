from akashi_core import (
    text,
    TextLayerParams,
    TextLayerType,
    KronArgs,
    Second
)
from math import sin, cos


def init(x: int, y: int, dur: Second) -> TextLayerParams:
    return TextLayerParams(
        begin=Second(0),
        end=dur,
        x=x,
        y=y,
        text='A',
        style={'fill': '#00FFFF', 'font_size': 120}
    )


def heart(init_x: int, init_y: int, t: Second, speed: int = 10) -> tuple[int, int]:
    computed_t = t * Second(speed)
    return (
        int(init_x + 10 * (16 * (sin(computed_t) ** 3))),
        int(init_y - 10 * (13 * cos(computed_t) - 5 * cos(Second(2) * computed_t) -
                           2 * cos(Second(3) * computed_t) - cos(Second(4) * computed_t)))
    )


def update(kronArgs: KronArgs, params: TextLayerParams) -> TextLayerParams:
    elapsed = kronArgs.playTime - params.begin
    new_x, new_y = heart(params.x, params.y, elapsed)

    params.x = new_x
    params.y = new_y

    return params


def message_layer(x: int, y: int, dur: Second = Second(3)) -> TextLayerType:
    return text(init=init(x, y, dur), update=update)
