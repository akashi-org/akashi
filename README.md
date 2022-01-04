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

...

@ak.entry()
def main():

    with ak.atom() as a1:
    
        with ak.lane() as _:
            ak.text('Sample Text').fit_to(a1).poly(
                lambda e, b, p: e(p.x) << e(p.x + 300) | e(p.y) << e(p.y + 300)
            )
            
        with ak.lane() as _:
        
            @gl.entry(ak.frag)
            def blue_filter(buffer: ak.frag, color: gl.frag_color) -> None:
                color.b *= 2.0
                
            ak.video('./dog.mp4').ap(
                lambda h: h.duration(10).stretch(True),
                lambda h: h.frag(blue_filter)
            )
```

## Installation

```bash
pip install akashi-engine
```

## Requirements

### System Requirements

* Linux (tested on Debian 11) 
* OpenGL core profile 4.2 or later
* Python 3.10 or later

### Runtime Dependencies

* FFmpeg 4.x
* SDL2, SDL_ttf, SDL_image
* Qt5

If you use Debian, you can install these dependencies by the command below.

```bash
apt install ffmpeg qtbase5-dev libsdl2-2.0-0 libsdl2-image-2.0-0 libsdl2-ttf-2.0-0
```

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

- Hardware Accerated Video Playback (VA-API)
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
from .config import akconfig

WIDTH = akconfig().video.resolution[0]
HEIGHT = akconfig().video.resolution[1]
VRES = list(map(lambda x: x // 2, akconfig().video.resolution))

def subtitle(msg: str, dur: float):

    ak.text(msg).ap(
        lambda h: h.pos(VRES[0], VRES[1] + 495),
        lambda h: h.duration(ak.sec(dur)),
        lambda h: h.fg(ak.Color.White, 40),
        lambda h: h.font_path('./myfont.ttf'),
    )

@ak.entry()
def main():

    with ak.atom() as a1:

        with ak.lane() as _:

            subtitle('Lorem ipsum dolor sit amet', 3)
            subtitle('At magnam natus ut mollitia reprehenderit', 3)

        with ak.lane() as _:

            ak.line(100).ap(
                lambda h: h.begin(0, VRES[1] + 500).end(VRES[0] * 2, VRES[1] + 500),
                lambda h: h.fit_to(a1),
            )

        with ak.lane() as _:

            ak.video('./blue_city.mp4').duration(3).pos(*VRES).stretch(True)
            ak.video('./cherry.mp4').duration(3).pos(*VRES).stretch(True)
```

### Circle Animation

https://user-images.githubusercontent.com/70841910/148137358-bd784005-84e2-48c7-9552-abeb638e73f1.mp4

<br>

```python
from akashi_core import ak, gl
from .config import akconfig
import random
random.seed(102)

WIDTH = akconfig().video.resolution[0]
HEIGHT = akconfig().video.resolution[1]
VRES = list(map(lambda x: x // 2, akconfig().video.resolution))

def random_radius() -> float:
    return random.choice([10, 20, 40, 80, 120])

def random_pos() -> tuple[int, int]:
    return (random.randrange(0, WIDTH), random.randrange(0, HEIGHT * 2))

def random_color() -> str:
    return ak.hsv(random.randrange(0, 360), 50, 100)

def circle_lane(radius: float, pos: tuple[int, int], color: str):
    with ak.lane() as _:

        @gl.entry(ak.poly)
        def fly(buffer: ak.poly, pos: gl.poly_pos) -> None:
            pos.y += buffer.time * 50

        ak.circle(radius).ap(
            lambda h: h.pos(*pos),
            lambda h: h.color(color),
            lambda h: h.poly(fly)
        )

@ak.entry()
def main():
    with ak.atom() as a1:
        for i in range(100):
            circle_lane(random_radius(), random_pos(), random_color())
        with ak.lane() as _:
            ak.rect(WIDTH, HEIGHT).fit_to(a1).pos(*VRES).color(ak.Color.White)
```
