from .utils import KERNEL_BIN_PATH

from akashi_core.config import config_parse

import argparse
from argparse import RawTextHelpFormatter
import sys
from os import path
from typing import Any
from dataclasses import dataclass
from subprocess import Popen, PIPE


def _version_string():
    # NB: pkg_resources is quite slow to import, so we import it only when it is really needed
    if '--version' not in sys.argv:
        return ''

    import pkg_resources
    py_ver = pkg_resources.get_distribution('akashi-engine').version
    _, err = Popen([KERNEL_BIN_PATH, "--version"], stderr=PIPE).communicate()
    engine_ver = err.decode('utf-8').split(' ')[-1]
    return f'akashi/{py_ver} engine-ver: {engine_ver}'


@dataclass(frozen=True)
class ParsedOption:
    action: str
    akconf: str
    conf_path: str
    asp_port: int


def argument_parse() -> ParsedOption:

    parser = argparse.ArgumentParser(
        prog="akashi",
        description="akashi",
        add_help=True,
        formatter_class=RawTextHelpFormatter)

    parser.add_argument(
        "action",
        choices=['run', 'build', 'kernel'],
    )

    parser.add_argument(
        "conf_path",
        type=str
    )

    parser.add_argument(
        "--version",
        action='version',
        version=_version_string()
    )

    if len(sys.argv) > 1 and sys.argv[1] == 'kernel':
        parser.add_argument(
            "-p", "--port",
            help="port number used for akashi-server",
            type=int,
            default='1234',
            required=False
        )

    args_dict = vars(parser.parse_args())
    if len(args_dict) == 0:
        parser.print_help(sys.stderr)
        sys.exit(0)

    return ParsedOption(
        args_dict['action'],
        config_parse(args_dict["conf_path"]).to_json(),
        path.abspath(args_dict["conf_path"]),
        args_dict['port'] if 'port' in args_dict else 1234
    )
