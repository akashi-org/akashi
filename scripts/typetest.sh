#!/bin/sh

if [ ! -f "setup.py" ]; then
  echo "this script must be executed in project root dir!"
  exit 1
fi

pyright -p pyrightconfig.test.json
