from .utils import KERNEL_BIN_PATH

import argparse
from argparse import RawTextHelpFormatter
import sys
from os import path
from importlib.machinery import SourceFileLoader
from typing import Any
from dataclasses import dataclass
from subprocess import Popen, PIPE
import pkg_resources


def _version_string():
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
        choices=['debug', 'build', 'kernel'],
    )

    parser.add_argument(
        "--conf_path",
        "-c",
        help="config file path",
        type=str,
        default="./akconf.py",  # temporary
        required=False
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
        _akconf_parse(args_dict["conf_path"]),
        path.abspath(args_dict["conf_path"]),
        args_dict['port'] if 'port' in args_dict else 1234
    )


def _akconf_parse(conf_path: str) -> str:

    akconf: Any = SourceFileLoader("akconf", path.abspath(conf_path)).load_module()  # type: ignore
    if not hasattr(akconf, '__akashi_export_config_fn'):
        raise Exception('No config function found. Perhaps you forget to add the export decorator?')
    return getattr(akconf, '__akashi_export_config_fn')().to_json()
