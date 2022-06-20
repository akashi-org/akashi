from akashi_core import ak


@ak.config()
def akconfig():
    return ak.AKConf(
        general=ak.GenerelConf(
            entry_file=ak.from_relpath(__file__, "./main.py"),
            include_dir=ak.from_relpath(__file__, "./")
        ),
        video=ak.VideoConf(
            fps=ak.sec(30),
            resolution=(1920, 1080),
            msaa=1
        ),
        audio=ak.AudioConf(
            format='flt',
            sample_rate=44100,
            channels=2,
            channel_layout='stereo'
        ),
        playback=ak.PlaybackConf(
            preferred_decode_method='vaapi',  # if you prefer software decoding, set 'sw' instead
        ),
        ui=ak.UIConf(
            resolution=(800, 450)  # initial resolution of the monitor
        ),
        encode=ak.EncodeConf(
            video_codec='libx264',
            audio_codec='aac'
        )
    )
