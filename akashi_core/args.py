from typing import Final

argv: Final[list[str]] = []


def _register_argv(v: str):
    global argv
    argv.clear()
    argv.extend(v.split(' '))
