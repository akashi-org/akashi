from .kron import (
    RootType,
    RootParams,
    RootInputParams,
    RootReceiver,
    SceneType,
    FrameContextGen,
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


def compute_frame_info(children: Sequence[SceneType], kron_args: KronArgs) -> _FrameInfo:
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
                    'duration': frame_info['duration'] + params._duration + (interval if idx > 0 else Second(0)),
                    'frame_cnt': frame_info['frame_cnt'] + params._frame_cnt + (1 if idx > 0 else 0),
                }),
            ),
        ))(kron_args)

    return frame_info


def root(_: RootInputParams, children: List[SceneType]) -> RootType:
    uuid = str(uuid4())

    def kron_func(receiver: RootReceiver = RootReceiver()) -> FrameContextGen:
        if receiver.get_children:
            receiver.get_children(children)

        def ctx_gen(kron_args: KronArgs):

            params = RootParams()
            frame_info = compute_frame_info(children, kron_args)
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

            acc = {"duration": Second(0)}
            interval = Second(1, kron_args.fps)
            res: List[FrameContext] = []
            for child in children:
                _res = child(KronReceiver(
                    mod_params=lambda params, _: begin(
                        (offset := acc['duration']),
                        acc.update({'duration': acc['duration'] + params._duration + interval}),
                        ret=replace(params, _offset=offset)
                    )
                ))(kron_args)

                # [TODO] we should make a comparison with play_time?
                if len(_res.layer_ctxs) > 0:
                    res.append(_res)
                    break

            return res[0] if len(res) > 0 else FrameContext(kron_args.playTime, [])

        return ctx_gen

    return kron_func
