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
            msaa=1,
            preferred_decode_method='vaapi',  # if you prefer software decoding, set 'sw' instead
        ),
        audio=ak.AudioConf(
            format='flt',
            sample_rate=44100,
            channels=2,
            channel_layout='stereo'
        ),
        playback=ak.PlaybackConf(
            gain=0.5  # 0 ~ 1.0
        ),
        ui=ak.UIConf(
            resolution=(800, 450)  # initial resolution of the monitor
        ),
        encode=ak.EncodeConf(
            # ffmpeg_format_opts="movflags=+faststart",

            video_codec='libx264', encode_method='sw',
            # video_ffmpeg_codec_opts="profile=high level=4.0 crf=22",

            # video_codec='h264_vaapi', encode_method='vaapi',
            # video_ffmpeg_codec_opts="profile=high level=4.0 qp=15",

            audio_codec='aac',
            # audio_ffmpeg_codec_opts="b=384k",
        )
    )
