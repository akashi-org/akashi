from .kron import (
    AtomType,
    AtomParams,
    AtomInputParams,
    AtomReceiver,
    LayerReceiver,
    KronArgs,
    FrameContext,
    LayerType,
    TLayerParams,
)

from .time import Second
from .lambda_utils import begin
from typing import Optional, List
from uuid import uuid4
from dataclasses import dataclass, replace


@dataclass(frozen=True)
class _FrameInfo:
    frame_cnt: int
    duration: Second


def compute_frame_info(duration: Second, fps: int) -> _FrameInfo:
    if duration == Second(0):
        return _FrameInfo(
            frame_cnt=1,
            duration=Second(0)
        )
    elif fps == 0:
        return _FrameInfo(
            frame_cnt=0,
            duration=Second(0)
        )
    else:
        duration_per_frame = Second(1, fps)
        _int = (duration / duration_per_frame).trunc()
        return _FrameInfo(
            frame_cnt=_int + 1,
            duration=duration_per_frame * Second(_int)
        )


def compute_begin(layer_params: TLayerParams, atom_params: AtomParams) -> Second:
    # [TODO] why we need this?
    if layer_params.begin > atom_params.duration:
        raise Exception('begin must be smaller than or equal to duration')
    if layer_params.begin < Second(0):
        raise Exception('begin must be larger than 0')

    return layer_params.begin + atom_params._offset


def compute_end(layer_params: TLayerParams, atom_params: AtomParams) -> Second:
    # [TODO] why we need this?
    if layer_params.end > atom_params.duration:
        raise Exception('end must be smaller than or equal to duration')
    if layer_params.end < Second(0):
        raise Exception('end must be larger than 0')
    if layer_params.end == Second(0):
        return atom_params.duration + atom_params._offset

    return layer_params.end + atom_params._offset


def compute_duration(
    children: List[LayerType],
    fps: int,
    duration: Optional[Second] = None
) -> Second:
    min_duration = Second(1, fps)

    if duration:
        if duration == Second(0):
            print('Atom duration must be larger than or equal to the value of 1 / fps')
            return min_duration
        else:
            return duration

    acc = {'max_duration': min_duration}

    for child in children:
        child(LayerReceiver(
            get_finalized_params=lambda params: begin(
                ret=acc.update({'max_duration': max(params.end, acc['max_duration'])}))
        ))

    return acc['max_duration']


def atom(inputs: AtomInputParams, children: List[LayerType]) -> AtomType:
    uuid = str(uuid4())

    def kron_func(receiver: AtomReceiver = AtomReceiver()):

        if receiver.get_children:
            receiver.get_children(children)

        def ctx_gen(kronArgs: KronArgs):

            params = AtomParams(
                Second(0) if 'duration' not in inputs.keys()
                else inputs['duration']
            )

            base_duration = compute_duration(
                children,
                kronArgs.fps,
                params.duration
            )

            frame_info = compute_frame_info(base_duration, kronArgs.fps)

            params.duration = frame_info.duration
            params._frame_cnt = frame_info.frame_cnt
            params._uuid = uuid

            finalized_params = (
                params if receiver.mod_params is None
                else receiver.mod_params(params, children)
            )
            del params

            if receiver.get_finalized_params:
                receiver.get_finalized_params(finalized_params)

            if receiver.get_frame_cnt:
                receiver.get_frame_cnt(frame_info.frame_cnt)

            if receiver.get_duration:
                receiver.get_duration(frame_info.duration)

            return FrameContext(
                pts=kronArgs.playTime,
                layer_ctxs=list(map(lambda child: child(LayerReceiver(
                    mod_params=lambda layer_params: replace(
                        layer_params,
                        begin=compute_begin(layer_params, finalized_params),
                        end=compute_end(layer_params, finalized_params),
                        _atom_uuid=uuid
                    )
                ))(kronArgs), children))
            )

        return ctx_gen

    return kron_func
