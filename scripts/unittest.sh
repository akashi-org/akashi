#!/bin/sh

if [ ! -f "setup.py" ]; then
  echo "this script must be executed in project root dir!"
  exit 1
fi

AK_LIBPROBE_PATH="./akashi_engine/build/libakprobe.so" python3 -u -m unittest discover
