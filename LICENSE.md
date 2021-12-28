Copyright (c) [2020-present] [crux14 <michaels310t@gmail.com>]



Akashi consists of multiple software components: **akashi**, **akashi-engine**, **akashi-probe**, and **akashi-core**. **akashi** is a Python CLI frontend of Akashi, and works together with **akashi-engine** through IPC. **akashi-engine**, written in C++, is the core engine of Akashi, and offers various features commonly expected as a video editor, including basic GUI interface, video playback, video editing, and scripting features. **akashi-engine** is single or multiple binary file(s), and can have arbitrary file names, such as *akashi_renderer*, *akashi_encoder*, *akashi_kernel*, and *akashi_engine*. **akashi-probe** is a native library to be dynamically linked as a C Extension in **akashi-core**. **akashi-core** is a core library of Akashi, which is written in Python. In Akashi, at every step of the video editing process, users write scripts using **akashi-core** which are supposed to be interpreted by **akashi-engine**.



Currently, these software components have different licenses. **akashi** and **akashi-core** are licensed under the ***Apache License 2.0***. **akashi-probe** is licensed under the ***GNU Lesser General Public License v3.0 or later***. **akashi-engine** is licensed under the ***GNU General Public License v3.0 or later***. Each license file should be stored somewhere in this project, named respectively `LICENSE.Apachev2`, `LICENSE.LGPLv3`, `LICENSE.GPLv3`.

