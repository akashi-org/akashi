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

## Demo

Although Akashi is still unstable, if conditions are met, you can try Akashi to see what it is like.

The current requirements are as follows.

### System Requirements

* Linux (tested on Debian 11) 
* X11
* Major desktop environments like Gnome
* Vim8
* OpenGL core profile 4.2 or later
* Python 3.9 or later

### Runtime Dependencies

* FFmpeg 4.x
* SDL2, SDL_ttf, SDL_image
* Qt5

If you use Debian, you can install these dependencies by the command below.

`apt install ffmpeg qtbase5-dev libsdl2-2.0-0 libsdl2-image-2.0-0 libsdl2-ttf-2.0-0 `

If your system satisfies the requirements, go to [Basic Example](https://github.com/akashi-org/akashi/examples/basic/) and follow the instructions there.
If you have any questions, please feel free to ask in issues page.

## Screenshots

![screenshot](https://user-images.githubusercontent.com/70841910/106698725-4aab9700-65d9-11eb-951c-9d751a741a99.png)

![sample1](https://user-images.githubusercontent.com/70841910/106697192-2ef2c180-65d6-11eb-8956-32208aed015b.gif)

## Tasks

- [x] Basic Playback Feature 
- [x] Basic Hot Reload Feature
- [x] Basic Shader Effects
- [x] Hardware Decoding(VA-API)
- [x] Basic Encoder
- [ ] Deployment by AppImage
- [ ] Introduce Unit Tests and E2E
- [x] Refactor Kron System
- [x] ASP Editing
- [ ] Documentation
- [ ] Qt -> GLFW

