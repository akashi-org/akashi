from akashi_core import (
    GenerelConf,
    VideoConf,
    AudioConf,
    PlaybackConf,
    UIConf,
    EncodeConf,
    AKConf,
    from_relpath,
    Second,
    akexport_config
)

'''
  Configuration File for Akashi

  The emoji `🚧` indicates that the setting should not be changed by the user as for now.
'''


@akexport_config()
def config():
    return AKConf(
        # General Settings
        # Please do not touch this for now
        general=GenerelConf(
            entry_file=from_relpath(__file__, "./main.py"),  # 🚧
            include_dir=from_relpath(__file__, "./"),  # 🚧
        ),
        # Video settings for the video which is supposed to be built
        video=VideoConf(
            fps=Second(24),
            resolution=(640, 360)
        ),
        # Audio settings for the video which is supposed to be built
        audio=AudioConf(
            format='flt',
            sample_rate=44100,
            channels=2,
            channel_layout='stereo'
        ),
        # Playback settings
        # This settings except for decode_method do not affect the encoding process.
        playback=PlaybackConf(
            gain=0.5,   # playback gain
            video_max_queue_size=1024 * 1024 * 100,  # 🚧
            audio_max_queue_size=1024 * 1024 * 30,  # 🚧
            decode_method='vaapi',  # if you prefer software decoding, set VideoDecodeMethod.SW
            video_max_queue_count=64  # 🚧
        ),
        # Monitor settings
        # This settings do not affect the encoding process.
        ui=UIConf(
            resolution=(640, 360),  # initial resolution of the monitor
            window_mode='split',
        ),
        # Encode settings
        # For the details, see the docs.
        encode=EncodeConf(
            out_fname="sample.mp4",
            video_codec='libx264',
            audio_codec='aac',
            encode_max_queue_count=10,  # 🚧
        )
    )
