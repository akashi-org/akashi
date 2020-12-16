#!/bin/sh

if [ ! -f "setup.py" ]; then
  echo "this script must be executed in project root dir!"
  exit 1
fi

cd akashi_engine/
FILES=$(find ./src/ -type f -regextype posix-extended -regex ".*(cpp|h)")
clang-format-10 -Werror -style=file --dry-run $FILES && (cd ..) || (cd .. && exit 1)
