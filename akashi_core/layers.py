from .kron import (
    VideoLayerParams,
    AudioLayerParams,
    TextLayerParams,
    ImageLayerParams,
    LayerUpdateFunc,
    VideoLayerType,
    AudioLayerType,
    TextLayerType,
    ImageLayerType,
    LayerReceiver,
    KronArgs,
    TLayerParams,
)
from .time import Second
from typing import Optional
from dataclasses import dataclass, fields
from uuid import uuid4


@dataclass(frozen=True)
class FrameInfo:
    frame_cnt: int
    duration: Second


def compute_frame_info(duration: Second, fps: int) -> FrameInfo:
    if duration == Second(0):
        return FrameInfo(
            frame_cnt=1,
            duration=Second(0)
        )
    elif fps == 0:
        return FrameInfo(
            frame_cnt=0,
            duration=Second(0)
        )
    else:
        duration_per_frame = Second(1, fps)
        _int = (duration / duration_per_frame).trunc()
        return FrameInfo(
            frame_cnt=_int + 1,
            duration=duration_per_frame * Second(_int)
        )


def __layer(
    init: TLayerParams,
    update: Optional[LayerUpdateFunc[TLayerParams]] = None
):
    uuid = str(uuid4())

    def kron_func(receiver: LayerReceiver[TLayerParams] = LayerReceiver()):

        _params = init
        comp_params = _params if receiver.mod_params is None else receiver.mod_params(_params)
        comp_params._display = True
        comp_params._uuid = uuid
        del _params

        if receiver.get_finalized_params:
            receiver.get_finalized_params(comp_params)

        def ctx_gen(kronArgs: KronArgs) -> TLayerParams:

            base_duration = comp_params.end - comp_params.begin
            frame_info = compute_frame_info(base_duration, kronArgs.fps)

            if receiver.get_frame_cnt:
                receiver.get_frame_cnt(frame_info.frame_cnt)

            if receiver.get_duration:
                receiver.get_duration(frame_info.duration)

            out_bound_begin = kronArgs.playTime < comp_params.begin
            out_bound_end = kronArgs.playTime > comp_params.end

            if not(out_bound_begin) and not(out_bound_end):
                new_params = comp_params if update is None else update(kronArgs, comp_params)
                # [XXX] be careful that init=False fields are not copied by dataclasses.replace!
                for field in fields(comp_params):
                    if not field.init:
                        setattr(new_params, field.name, getattr(comp_params, field.name))
                return new_params
            else:
                comp_params._display = False
                return comp_params

        return ctx_gen

    return kron_func


def video(
    init: VideoLayerParams,
    update: Optional[LayerUpdateFunc[VideoLayerParams]] = None
) -> VideoLayerType:
    return __layer(init, update)


def audio(
    init: AudioLayerParams,
    update: Optional[LayerUpdateFunc[AudioLayerParams]] = None
) -> AudioLayerType:
    return __layer(init, update)


def text(
    init: TextLayerParams,
    update: Optional[LayerUpdateFunc[TextLayerParams]] = None
) -> TextLayerType:
    return __layer(init, update)


def image(
    init: ImageLayerParams,
    update: Optional[LayerUpdateFunc[ImageLayerParams]] = None
) -> ImageLayerType:
    return __layer(init, update)
