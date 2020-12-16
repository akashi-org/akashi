from __future__ import annotations
from .kron import (
    AtomType,
    LayerType,
    KronType,
    SceneType,
    KronName,
    KronParams,
    KronArgs,
    KronReceiver,
    LayerParams,
    LayerReceiver,
    VideoLayerParams,
    AudioLayerParams,
    FrameContext,
    TKronType,
    TKronChild_co,
    KronChildType
)
from .time import Second
from typing import List, Union, Literal, cast, Sequence
from dataclasses import dataclass, field
from uuid import uuid4


@dataclass
class RenderProfile:
    duration: Second
    uuid: str
    atom_profiles: List[AtomProfile]


@dataclass
class AtomProfile:
    begin: Second = field(default=Second(0), init=False)
    end: Second = field(default=Second(0), init=False)
    duration: Second
    uuid: str
    layers: List[LayerProfile]


@dataclass
class LayerProfile:
    type: Literal['VIDEO', 'AUDIO']
    begin: Second
    end: Second
    uuid: str
    src: str
    start: Second


def get_kron_type(kron: Union[KronType, LayerType]) -> Union[KronName, Literal['LAYER']]:

    if kron.__qualname__.startswith('root.'):
        return 'ROOT'
    elif kron.__qualname__.startswith('scene.'):
        return 'SCENE'
    elif kron.__qualname__.startswith('atom.'):
        return 'ATOM'
    else:
        return 'LAYER'


def get_kron_duration(kron: TKronType, kron_args: KronArgs) -> Second:

    duration = [Second(0)]

    def get_duration(_duration: Second):
        duration[0] = _duration

    kron(KronReceiver(
        get_duration=get_duration
    ))(kron_args)

    return duration[0]


def get_frame_contexts(
        kron: TKronType, start_time: Second, fps: int, duration: Second, length: int) -> List[FrameContext]:
    frame_ctxs: List[FrameContext] = []

    for i in range(length):
        frame_ctx = kron(KronReceiver())(
            KronArgs(playTime=start_time + (Second(i) * Second(1, fps)), fps=fps))
        if frame_ctx.pts <= duration:
            frame_ctxs.append(frame_ctx)

    diff = length - len(frame_ctxs)
    if diff <= 0:
        return frame_ctxs
    else:
        return frame_ctxs + get_frame_contexts(kron, Second(0), fps, duration, diff)


def get_render_profile(kron: TKronType, kron_args: KronArgs) -> RenderProfile:

    return RenderProfile(
        duration=get_kron_duration(kron, kron_args),
        uuid=str(uuid4()),
        atom_profiles=get_atom_profiles(kron, kron_args)
    )


def get_atom_profiles(kron: TKronType, kron_args: KronArgs) -> List[AtomProfile]:

    partial_atom_profiles = get_partial_atom_profiles(kron, kron_args)
    interval = Second(1, kron_args.fps)
    acc_duration = Second(0)

    for partial_atom_profile in partial_atom_profiles:
        partial_atom_profile.begin = acc_duration
        partial_atom_profile.end = acc_duration + partial_atom_profile.duration
        for layer in partial_atom_profile.layers:
            layer.begin += acc_duration
            layer.end += acc_duration
        acc_duration = partial_atom_profile.end + interval

    return partial_atom_profiles


def get_partial_atom_profiles(kron: TKronType, kron_args: KronArgs) -> List[AtomProfile]:

    partial_atom_profiles: List[AtomProfile] = []

    def get_children(children: Sequence[TKronChild_co]) -> None:
        for child in children:
            if get_kron_type(cast(KronChildType, child)) == 'SCENE':
                partial_atom_profiles.extend(
                    get_partial_atom_profiles(cast(SceneType, child), kron_args)
                )
            elif get_kron_type(cast(KronChildType, child)) == 'ATOM':
                partial_atom_profiles.extend(
                    get_partial_atom_profiles(cast(AtomType, child), kron_args)
                )

    def get_finalized_params(params: KronParams) -> None:
        if params._type == 'ATOM':
            partial_atom_profiles.append(AtomProfile(
                duration=params.duration,
                uuid=params._uuid,
                layers=get_layer_profiles(cast(AtomType, kron), kron_args),
            ))

    kron(KronReceiver(
        get_children=get_children,
        get_finalized_params=get_finalized_params
    ))(kron_args)

    return partial_atom_profiles


def get_layer_profiles(kron: Union[AtomType, LayerType], kron_args: KronArgs) -> List[LayerProfile]:

    layer_profiles: List[LayerProfile] = []

    if get_kron_type(kron) == 'ATOM':
        def get_children(children: Sequence[LayerType]) -> None:
            for child in children:
                layer_profiles.extend(get_layer_profiles(child, kron_args))

        cast(AtomType, kron)(KronReceiver(
            get_children=get_children,
        ))(kron_args)

    else:
        def get_finalized_params(params: LayerParams) -> None:
            if params._type == 'VIDEO' or params._type == 'AUDIO':
                _params = cast(Union[VideoLayerParams, AudioLayerParams], params)
                layer_profiles.append(LayerProfile(
                    begin=_params.begin,
                    end=_params.end,
                    type=params._type,
                    uuid=_params._uuid,
                    src=_params.src,
                    start=_params.start
                ))

        cast(LayerType, kron)(LayerReceiver(
            get_finalized_params=get_finalized_params
        ))(kron_args)

    return layer_profiles
