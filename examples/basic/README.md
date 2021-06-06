# Basic Example

<br>

## Requirements

### System Requirements

* Linux (tested on Debian 11) 
* X11
* Major desktop environments like GNOME
* Vim8
* OpenGL core profile 4.2 or later
* Python 3.9 or later
* PulseAudio

### Runtime Dependencies

* FFmpeg 4.x
* SDL2, SDL_ttf, SDL_image

<br>

## Disclaimer

Akashi is still unstable and might have some risks. For instance, Akashi now suffers from minor memory leaks and infrequent noises. In most cases, no matter what bad things happen, rebooting your system will suffice for fixing the problems. But you should be warned about the risks before using.

In addition, I am planning to replace the current Python frontend with another one. In a few months, this example can be completely outdated. The motivation of this example solely lies in giving users a chance to tinker around with Akashi, not in education. So, please do not try to learn too much.

<br>

## Try Akashi Step-by-Step

### Step1: Install akashi-engine and ASP Client

First off, you will need to install `akashi-engine`. 
Since `akashi-engine` is distributed in PyPI, you can install it by the command:

```bash
pip install akashi-engine --pre
```

Akashi is a tool that uses a text editor to edit videos, so you will need a text editor. Akashi introduces **ASP** (*Akashi Server Protocol*), which is an LSP-like protocol to enable seamless integration between text editors and the engine. Like LSP, **ASP** itself is **technically independent of text editors**, but it requires a client plugin for actual use. As for now, the only client implemented is for Vim, so if you want to try this example, you will need to install `akashi.vim`. 

You can install `akashi.vim` just as you would any other vim plugins.
For example, if you use [Vim-Plug](https://github.com/junegunn/vim-plug), add the line below to your `~/.vimrc`:

```vim
Plug 'akashi-org/akashi.vim'
```

Then run the command `:source %`, and `:PlugInstall` in Vim.

### Step2: Scripting Overview

Before getting into the specifics, let's briefly review the basics of scripting in Akashi.

To represent the structures of video, existing video editors use a timeline GUI, on the other hand, Akashi does not rely on GUI and uses scripts with four elements: `root`, `scene`, `atom`, and `layer`. `root` represents the entire video that will eventually be built, and is composed of smaller units of it, `scene`. `scene` is further divided into smaller chunks, `atom`. As for now, only these elements are the units which can be played back stand-alone. These three elements represent the logical structures of video, while `layer`, which belongs to `atom`, represents the graphical structures of video. Currently, there are three types of `layer` implemented: `video`, `image`, and `text`.

In the actual implementation, these four elements are functions, and are used in combination as follows. 

```python
@akexport_elem()
def main():
    return root({}, [
        scene({}, [
            atom({}, [
                video(init=VideoLayerParams(
                    src=from_relpath(__file__, 'assets/river.mp4'),
                    x=int(WIDTH / 2),
                    y=int(HEIGHT / 2),
                    begin=Second(0),
                    end=Second(10),
                ))
            ])
        ])
    ])
```

The decorator `@akexport_elem()` indicates that the decorated function generates elements which should be played back by the engine.

### Step3: Quick Tour

Now, let us go through the steps of the video editing in Akashi.

Firstly, I advise you to setup ASP Client. For the settings and the details, please see [Appendix-A](#appendix-a-configuration-of-asp-client) and [Appendix-B](#appendix-b-asp-commands).  
If it's too much trouble for you, you can skip this setting.

Once you have an ASP Client setting setup, execute:

```bash
cd examples/basic/
```

and open `main.py` in vim and execute `:ASPInit akconf.py` in vim.

Wait a short sec, and execute `:ASPEval`. Then `akashi-player` shows up. To start playback, execute `:ASPTogglePlayState`. To pause, execute `:ASPTogglePlayState` again. With it paused, let's rewrite `main.py` as follows.

```python
@akexport_elem()
def main():
    return root({}, [
        scene({}, [
            atom({}, [
                message_layer(int(WIDTH / 2), int(HEIGHT / 2), Second(5)), # uncomment this line
                video(init=VideoLayerParams(
                    src=from_relpath(__file__, 'assets/river.mp4'),
                    x=int(WIDTH / 2),
                    y=int(HEIGHT / 2),
                    begin=Second(0),
                    end=Second(10),
                    frag_path=from_relpath(__file__, './video.frag')
                ))
            ])
        ])

    ])
```

Then you can see the image on the player updates. All the codes in Akashi, except for `akconf.py`, are hot reloadable. So the changes on them will be reflected instantly on the player.

`akashi-player` provides several basic GUI features like playback-toggle, fullscreen-toggle, and seeking. To toggle fullscreen, double-click the player. To close, click the cross icon on the top-left corner. Since `Focus passing` feature is enabled by default on the player, you can still use hotkeys of your text editors. These GUI features **are supposed to be deprecated** in the future as Akashi becomes more stable.

### Step4: Configuration 

You also need a configuration file on the project root, which is `akconf.py` in this example. The configuration file must have a single function decorated by `@akexport_config()` as follows.

```python
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
        # Player settings
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
```

For the details of configuration, please check the comments in `akconf.py`. 

Here we take a look on several related topics on this.

#### Hardware Decoding

Akashi supports VA-API hardware decoding. If your GPU supports VA-API, you can enable it by setting `decode_method` in `PlayBackConf` to `'vaapi'`.

#### Window mode

You can configure window mode in `akconf.py`. About window mode, please see [Appendix C](#appendix-c-features-of-akashi-player).

### Step5: Editing

As explained above, in Akashi, all the graphical structures of video are represented by `layer`.

In the implementation aspect, `layer` is a function, which has two arguments: `init` and `update`. `init` is a required argument to be filled by layer parameters. `update` is an optional argument to be filled by an update function. Below, we will look further into the details of the two arguments.

#### Layer Parameters

##### Common Parameters

All the layers have some common parameters, `begin`, `end`, `x`, and `y`.  

`begin` and `end` are options to specify from when to when a layer exists in the atom to which it belongs. The type of the two parameters must be `Second`.   Actually, `Second` is just a rational number type. For example, for `Second(1.5)` which stands for 1.5 sec, the expression below is truthy.

```python
Second(1.5) == Second(3, 2)
``` 

Thus, the example below indicates that the video layer exists between 1.5 and 10 seconds within the atom to which the layer belongs.

```python
video(init=VideoLayerParams(
    x=(WIDTH // 2),
    y=(HEIGHT // 2),
    begin=Second(3, 2),
    end=Second(10),
))
```

`x` and `y` are options to specify the initial x-y coordinates of a layer. `x` and `y` must have an integer type.

##### Video Layer Parameters

You can specify the source video path with a `src` option. This option is required.  
A `start` option refers to the offset of the source video's start position.  

```python
video(init=VideoLayerParams(
    x=(WIDTH // 2),
    y=(HEIGHT // 2),
    src="sample.mp4",
    start=Second(55),
    begin=Second(3, 2),
    end=Second(10),
))
````

The difference between the latter and the former case is that in this case frames from 56.5 seconds to 65 seconds will be used because `begin` and `end` will be offset at 55 seconds, which is the playback time in `sample.mp4`. Please note that even in this case the video layer will exist between 1.5 and 10 seconds within the atom.

##### Image Layer Parameters

You can specify the image path with a `src` option. This option is required.  
For `video`, `text`, and `image` layer, a `scale` option is defined so that users can change the scale value of the layer size.  
If you set 2.0 to the value of `scale`, the size of the layer will be magnified by 2x.  

```python
image(init=ImageLayerParams(
    src="sample.png",
    x=(WIDTH // 2),
    y=(HEIGHT // 2),
    begin=Second(0),
    end=Second(10),
    scale=2.0
))
```

##### Text Layer Parameters

You can specify the text content with a `text` option. This option is required.  
You can also configure the style of it by a `style` option like this.  

```python
style={'fill': '#FF0000', 'font_size': 30, 'font_path': '/path/to/font.ttf`}
```

#### Layer update 

As the second argument of `layer`, you can optionally pass an update function. The update function is executed every frame. This function accepts `KronArgs`, which contains the PTS (Presentation timestamp) of the frame, and the initial layer parameters as its arguments, and returns the new layer parameters used for rendering for that turn. This function must be pure: you must not perform side effects in this function. This function is useful when you want to change the layer's location with time. [This example](https://github.com/akashi-org/akashi/blob/master/examples/basic/layers/message_layer.py) is a good example for this.

#### Shader

You can also customize layers with GLSL (OpenGL Shading Language). GLSL is a really powerful tool for Akashi. Unlike layer updates, code changes for shaders can be applied immediately even during the playback. And since it runs on the GPU, the performance is way better than that of layer updates. To see what you can do with GLSL in detail, see [this example](https://github.com/akashi-org/akashi/blob/master/examples/basic/video.frag).

Akashi now supports two types of shaders: geometry and fragment shader.

```python
video(init=VideoLayerParams(
    src=from_relpath(__file__, 'assets/river.mp4'),
    x=int(WIDTH / 2),
    y=int(HEIGHT / 2),
    begin=Second(0),
    end=Second(10),
    frag_path=from_relpath(__file__, './video.frag')
))
```

When using, specify the shader file path like above. Make sure to install shader files within the directory where `akconf.py` exists. If not, hot reloading is not working.


### Step6: Encoding (Build)

Although still in an experimental stage, Akashi supports a video encoding feature. For settings, we use `akconf.py`. Basically, only `VideoConf`, `AudioConf`, `EncodeConf` and some settings of `PlaybackConf` (such as `decode_method`) are involved in the encoding settings.

Here is a detailed explanation of `EncodeConf`.

```python
        # Encode settings
        # For the details, see the docs.
        encode=EncodeConf(
            out_fname="sample.mp4",
            video_codec='libx264',
            audio_codec='aac',
            encode_max_queue_count=10,  # 🚧
        )
```

You can specify the output video path in `out_fname`. Please note that the extension of `out_frame` will be used for the video container detection in the encoding process. `video_codec` and `audio_codec` are used for specifying the codec string.  

You can obtain the codec string by the command:

```bash
ffmpeg -encoders
```

When using VP8, type

```bash
ffmpeg -encoders 2> /dev/null | grep vp8
```

Then, you can get the outputs below.

```bash
V..... libvpx               libvpx VP8 (codec vp8)
V..... vp8_v4l2m2m          V4L2 mem2mem VP8 encoder wrapper (codec vp8)
V..... vp8_vaapi            VP8 (VAAPI) (codec vp8)
```

In this case, the codec strings are `libvpx` or `vp8_v4l2m2m` or`vp8_vaapi`. Usually you should choose `libvpx`.

Encoding itself is done by the CLI tool `akashi`, which is automatically installed when you install `akashi-engine`.  
The encoding process is started by the command:

```bash
akashi build -c akconf.py
```


<br>

## Known Bugs and Gotchas

### *Do not invoke HR for Python when playing*

Unlike GLSL, hot reload on python sources during playback is unstable. Before hot reloading, please make sure to pause the player.  
Currently, if a hot reload detected for python sources, engine will pause the player automatically.  

### *Do not use `dataclasses.replace` in layer update*

**DON'T DO**

```python
def update(kronArgs: KronArgs, params: TextLayerParams) -> TextLayerParams:
    elapsed = kronArgs.playTime - params.begin
    new_x, new_y = heart(params.x, params.y, elapsed)
    return dataclasses.replace(params, x=new_x, y=new_y)
```

**DO**

```python
def update(kronArgs: KronArgs, params: TextLayerParams) -> TextLayerParams:
    elapsed = kronArgs.playTime - params.begin
    new_x, new_y = heart(params.x, params.y, elapsed)
    params.x = new_x
    params.y = new_y
    return params
```

### *Do not use `__init__.py` in akashi projects*

Currently, there is a bug that hot reload does not work properly when `__init__.py` exists in a project directory tree.

### *Keep your codebase fully typed as much as possible as you can*

Akashi makes use of the power of type hints in Python for maximizing the developer experience.

### *Seeking is basically unstable*

After seeking, you might see that the player is not updated properly. In that case, seeking again will fix the problem.

### *Use ASPPlayerExit && ASPEval*

When something strange happens, use `:ASPPlayerExit` and restart the player by `:ASPEval`

### *Use ASPLog*

When something strange happens, use `:ASPLog` to see what happens.

<br>

## Appendix A: Configuration of ASP Client

After installing `akashi.vim`, you can use ASP commands by calling them directly. But in practice, you will be advised to setup hotkeys for them. Currently, `akashi.vim` does not have default key bindings for ASP commands, so you must register them for yourself. 

Below is the example.

```vim
nnoremap <S-Right> :ASPRSeek(0.1)<CR>
nnoremap <S-Left> :ASPRSeek(-0.1)<CR>
nnoremap <C-Right> :ASPFrameStep<CR>
nnoremap <C-Left> :ASPFrameBackStep<CR>
nnoremap <leader>p :ASPTogglePlayState<CR>
nnoremap <leader>f :ASPToggleFullscreen<CR>
nnoremap <leader>t :ASPCurrentTime<CR>
nnoremap <F8> :ASPEval<CR>
nnoremap <S-F8> :ASPPlayerExit<CR>
```

<br>

## Appendix B: ASP Commands

### `ASPInit`

This command spawns the `akashi-engine` process.  
This command expects the single argument which is the path of `akconf.py`.  
Almost all of the other commands require that you have run this command beforehand.  

### `ASPTerminate`

This command terminates the `akashi-engine` process.  
This command requires that you have run `:ASPInit` beforehand.  

### `ASPPlayerExit`

This command closes `akashi-player`.  
This command requires that you have run `:ASPInit` beforehand.  

### `ASPLog`

This command shows logs from `akashi-engine`.  

### `ASPEval`

This command opens `akashi-player` 
This command requires that you have run `:ASPInit` beforehand.  

### `ASPSeek`

This command emits an absolute seeking command to the `akashi-engine`.  
This command requires that you have run `:ASPInit` beforehand.  
This command expects the single argument which is a positive integer value by second.  

If `:ASPSeek(10)` executed,  it will seek to the position at 10 seconds in the video.  

### `ASPRSeek`

This command emits an relative seeking command to the `akashi-engine`.  
This command requires that you have run `:ASPInit` beforehand.  
This command expects the single argument which is an float value ranging from -1.0 to 1.0, inclusive.  

If `:ASPSeek(0.1)` executed, from the current playback position, it will seek to the latter position by 10% relative to the total duration.  
If `:ASPSeek(-0.1)` executed, from the current playback position, it will seek to the former position by 10% relative to the total duration.  

### `ASPFrameStep`

This command emits a command to seek one frame ahead to the `akashi-engine`.  
This command requires that you have run `:ASPInit` beforehand.  

### `ASPFrameBackStep`

This command emits a command to seek one frame behind to the `akashi-engine`.  
This command requires that you have run `:ASPInit` beforehand.  

### `ASPCurrentTime`

This command fetches the current playback time, and copies it to the clipboard.  
This command requires that you have run `:ASPInit` beforehand.  

### `ASPToggleFullscreen`

This command emits a command to toggle fullscreen to the `akashi-engine`.  
This command requires that you have run `:ASPInit` beforehand.  

### `ASPTogglePlayState`

This command emits a command to toggle playback state to the `akashi-engine`.  
This command requires that you have run `:ASPInit` beforehand.  

### `ASPChangePlayVolume`

This command emits a command to change the playback volume to the `akashi-engine`.  
This command requires that you have run `:ASPInit` beforehand.  
This command expects the single argument that is a positive float value.

<br>

## Appendix C: Features of Akashi Player

`akashi-player` has several features that you should know.

### Window Mode

The first one is `window mode`.
As for its monitor's behavior, `akashi-player` has three window modes, `Split mode`, `Immersive mode`, and `Independent mode`.

#### Split mode

`Split mode` is the default window mode, and is best for working with a single display. The monitor in this mode is set to be transient for the text editor, and when the editor gets hidden, the monitor also gets hidden automatically. You can say that the monitor in this mode behaves like a child window for the text editor. In addition, the monitor in this mode can get focus, but the focus will be passed instantly to the text editor. This feature is called `Focus passing` in Akashi, and the details are described later.

#### Immersive mode

`Immersive mode` is also best for working with a single display. Like `Split mode`, the monitor in this mode is set to be transient, and `Focus passing` feature is enabled for this. The difference is that fullscreen mode is always enabled for `Immersive mode`.
So There are two looks for this mode: transparent fullscreen and opaque fullscreen. In this mode, `ASPToggleFullscreen` is to toggle these two looks. **This mode can potentially induce cybersickness. If you feel sick, please stop using immediately.**

#### Independent mode

`Independent mode` is best for working with a single or multi-display.
Unlike `Split mode`, this mode is not transient, and `Focus passing` feature is disabled.

### Focus passing 

This feature was created to enhance the integration between the editor and the player. When this feature is enabled, the player in this mode can get focus, but the focus will be passed instantly to the editor. With this feature, you can use your text editor's hotkeys even when the monitor gets fullscreen. However, this feature is a little unstable, and sometimes the passing can be failed. When hotkeys don't work on fullscreen, just double-click the player to make it non-fullscreen, and then manually focus on the editor. When the focus gets stuck within the player, manually focus on the editor.
