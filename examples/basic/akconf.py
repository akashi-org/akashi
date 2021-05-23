from akashi_core import (
    GenerelConf,
    VideoConf,
    AudioConf,
    PlaybackConf,
    UIConf,
    EncodeConf,
    AudioSampleFormat,
    AudioChannelLayout,
    AKConf,
    VideoDecodeMethod,
    from_relpath,
    WindowMode,
    Second,
    akexport_config
)


@akexport_config()
def config():
    return AKConf(
        general=GenerelConf(
            entry_file=from_relpath(__file__, "./main.py"),
            include_dir=from_relpath(__file__, "./"),
        ),
        video=VideoConf(
            fps=Second(24),
            resolution=(1920, 1080)
        ),
        audio=AudioConf(
            format=AudioSampleFormat.FLT,
            sample_rate=44100,
            channels=2,
            channel_layout=AudioChannelLayout.STEREO
        ),
        playback=PlaybackConf(
            gain=0.5,
            video_max_queue_size=1024 * 1024 * 100,
            audio_max_queue_size=1024 * 1024 * 30,
            decode_method=VideoDecodeMethod.VAAPI,
            video_max_queue_count=64
        ),
        ui=UIConf(
            resolution=(640, 360),
            window_mode=WindowMode.SPLIT,
        ),
        encode=EncodeConf(
            out_fname="sample.mp4",
            video_codec='libx264',
            audio_codec='aac',
            encode_max_queue_count=10,
        )
    )
