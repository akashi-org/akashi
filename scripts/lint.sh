#!/bin/sh

if [ ! -f "setup.py" ]; then
  echo "this script must be executed in project root dir!"
  exit 1
fi

python3 -m flake8 --config .flake8 akashi_cli/ akashi_core/
