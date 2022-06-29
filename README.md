<p align="center"><img width="150" src="https://user-images.githubusercontent.com/70841910/115134602-16088280-a001-11eb-991e-a091139b6a25.png" /></p>

<h1 align="center">Akashi</h1>
<p align="center">
  <strong>A Next-Generation Video Editor</strong>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/license-GPLv3%2FApache%202-blue" alt="GPLv3/Apachev2" />
</p>
<br>

**Akashi** is a next-generation video editor. 

You can edit your videos only by programs without being bothered with complex GUI.

Akashi is still in the very early stages of development, and **not ready for production** in any sense.

## Example

```python
from akashi_core import ak, gl

@ak.entry()
def main():

    ak.text('Sample Text', lambda t: (
        t.text.fg('#00ff00', 100),
        t.duration(ak.root),
    ))

    @gl.entry(ak.frag)
    def blue_filter(buffer: ak.frag, color: gl.frag_color) -> None:
        color.b *= 2.0

    ak.video(ak.from_relpath(__file__, './dog.mp4'), lambda t: (
        t.duration(10),
        t.transform.layer_size(ak.width(), -1),
        t.frag(blue_filter)
    ))
```

## Installation

TBD

## Requirements

### System Requirements

* Linux (tested on Debian 11) 
* OpenGL core profile 4.2 or later
* Python 3.10 or later

### Runtime Dependencies

* FFmpeg 4.x
* SDL2, SDL_ttf, SDL_image
* Qt5

*Note*: Windows support is planned for a future release.

## Getting Started

TBD

## Features

### Code Driven Development

Video Editing in Akashi is completely code driven.  
You no longer need complex GUIs like timeline and inspectors.

### Rich and Fast Visual Effects by GPU Shaders

In Akashi, every step of the video editing process is dominated in Python.  
The Python codes on visual effects are compiled to shaders by the JIT compiler.  
This gives Python a super power which shaders have; performance far beyond any other languages and non-comparable expressions.

### Modern Python

Type hinting is mandatory in Akashi, and we require you to do so.  
Basically, all errors are detected before runtime, and you will have a nice Python IntelliSense experience.  
This helps you write your codes more secure and easier.

### New-Fashioned User Interface

- Built-in Hot Reload
- Seamless Editing by ASP
- 🚧 Smart GUI
   
### Basic Video Editing Features

- Hardware Accerated Video Playback / Encoding (VA-API)
- Audio Playback (PulseAudio)
- Image Rendering
- Text Rendering (ttf/otf, outline, shadow, etc.)
- Basic 2D shapes (Rectangle, Circle, Round Rectangle, Triangle, Line, etc.)
- Rich Codec Backend (FFmpeg)
- Video Encoding

## Gallery

### Two Videos With Subtitles

https://user-images.githubusercontent.com/70841910/148137328-02665a2e-962a-4d82-9414-66fa94cc196e.mp4

<br>

```python

from akashi_core import ak, gl

def subtitle(msg: str, dur: float):

    ak.text(msg, lambda t: (
        t.duration(ak.sec(dur)),
        t.text.fg(ak.Color.White, 32),
        t.text.font_path('./myfont.ttf'),
    ))


def layout(layout_ctx: ak.LayoutLayerContext):

    match layout_ctx.key:
        case 'subtitle':
            subt_height = int(ak.height() * 0.1)
            pos = (ak.center()[0], ak.height() - subt_height // 2)
            return ak.LayoutInfo(pos=pos)
        case _:
            return None

@ak.entry()
def main():

    with ak.unit() as u1:

        u1.layout(layout)

        with ak.scene(ak.width(), int(ak.height() * 0.1)) as s1:
            s1.key('subtitle')
            s1.bg_color(ak.Color.Black)

            subtitle('Lorem ipsum dolor sit amet', 3)
            subtitle('At magnam natus ut mollitia reprehenderit', 3)

        with ak.scene() as s2:
            s2.key('main_video')

            ak.video('./blue_city.mp4', lambda t: t.media.span_dur(3))
            ak.video('./cherry.mp4', lambda t: t.media.span_dur(3))
```

### Circle Animation

https://user-images.githubusercontent.com/70841910/148137358-bd784005-84e2-48c7-9552-abeb638e73f1.mp4

<br>

```python
from akashi_core import ak, gl
import random
random.seed(102)

def random_radius() -> float:
    return random.choice([10, 20, 40, 80, 120])

def random_pos() -> tuple[int, int]:
    return (random.randint(0, ak.width()), random.randint(0, ak.height() * 2))

def random_color() -> str:
    return ak.hsv(random.randint(0, 360), 50, 100)

def circle_layer(radius: float, pos: tuple[int, int], color: str):

    @gl.entry(ak.poly)
    def fly(buffer: ak.poly, pos: gl.poly_pos) -> None:
        pos.y += buffer.time * 50

    ak.circle(radius, lambda t: (
        t.transform.pos(*pos),
        t.shape.color(color),
        t.poly(fly)
    ))

@ak.entry()
def main():

    ak.root.bg_color(ak.Color.White)

    for i in range(100):
        circle_layer(random_radius(), random_pos(), random_color())
```
