from .kron import (
    SceneType,
    SceneParams,
    SceneInputParams,
    SceneReceiver,
    AtomType,
    KronArgs,
    FrameContext,
    KronReceiver,
)
from .time import Second
from .lambda_utils import begin

from dataclasses import replace
from typing import Sequence, TypedDict, List
from uuid import uuid4


class _FrameInfo(TypedDict):
    frame_cnt: int
    duration: Second


def compute_frame_info(children: Sequence[AtomType], kron_args: KronArgs) -> _FrameInfo:
    frame_info: _FrameInfo = {
        "frame_cnt": 0,
        "duration": Second(0)
    }

    fps = kron_args.fps
    interval = Second(1, fps)

    for idx, child in enumerate(children):
        child(KronReceiver(
            get_finalized_params=lambda params: begin(
                ret=frame_info.update({
                    'duration': frame_info['duration'] + params.duration + (interval if idx > 0 else Second(0)),
                    'frame_cnt': frame_info['frame_cnt'] + params._frame_cnt + (1 if idx > 0 else 0),
                })
            )
        ))(kron_args)

    return frame_info


def scene(inputs: SceneInputParams, children: List[AtomType]) -> SceneType:
    uuid = str(uuid4())

    def kron_func(receiver: SceneReceiver = SceneReceiver()):
        if receiver.get_children:
            receiver.get_children(children)

        def ctx_gen(kron_args: KronArgs):

            params = SceneParams(None if 'path' not in inputs.keys() else inputs['path'])

            frame_info = compute_frame_info(children, kron_args)

            params._duration = frame_info['duration']
            params._frame_cnt = frame_info['frame_cnt']
            params._uuid = uuid

            finalized_params = (
                params
                if receiver.mod_params is None
                else receiver.mod_params(params, children)
            )
            del params

            if receiver.get_finalized_params:
                receiver.get_finalized_params(finalized_params)

            if receiver.get_frame_cnt:
                receiver.get_frame_cnt(frame_info['frame_cnt'])

            if receiver.get_duration:
                receiver.get_duration(frame_info['duration'])

            outer_acc = {"duration": Second(0)}
            interval = Second(1, kron_args.fps)
            res: List[FrameContext] = []
            for child in children:

                inner_acc = {
                    "offset": Second(0),
                    "begin": Second(0),
                    "end": Second(0)
                }

                _res = child(KronReceiver(
                    mod_params=lambda params, _: begin(
                        inner_acc.update(
                            {'offset': outer_acc['duration'] + finalized_params._offset}),
                        outer_acc.update(
                            {'duration': outer_acc['duration'] + params.duration + interval}),
                        ret=replace(params, _offset=inner_acc['offset'])
                    ),
                    get_finalized_params=lambda child_final_params: begin(
                        inner_acc.update({'begin': child_final_params._offset}),
                        inner_acc.update({'end': inner_acc['begin'] + child_final_params.duration}),
                        ret=None
                    )
                ))(kron_args)

                if inner_acc['begin'] <= kron_args.playTime <= inner_acc['end']:
                    res.append(_res)
                    break

            return res[0] if len(res) > 0 else FrameContext(kron_args.playTime, [])

        return ctx_gen

    return kron_func
