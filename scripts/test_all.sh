#!/bin/sh

if [ ! -f "setup.py" ]; then
  echo "this script must be executed in project root dir!"
  exit 1
fi

exec_script() {
  tag=$1
  script_path=$2

  echo "[$tag] ...executing"
  if sh "$script_path" ; then
    echo "[$tag] \e[32msuccess\e[m"
  else
    echo "[$tag] \e[31mfailed\e[m"
    exit 1
  fi
}

exec_script "lint" "./scripts/lint.sh"
exec_script "typecheck" "./scripts/typecheck.sh"
exec_script "unittest" "./scripts/unittest.sh"
exec_script "engine-lint" "./scripts/engine_lint.sh"
