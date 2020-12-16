import argparse
from argparse import RawTextHelpFormatter
import sys
from os import path
from importlib.machinery import SourceFileLoader
from typing import Any


def argument_parse() -> str:

    parser = argparse.ArgumentParser(
        prog="akashi-cli",
        description="akashi-cli",
        add_help=True,
        formatter_class=RawTextHelpFormatter)

    parser.add_argument(
        "--conf_path",
        "-c",
        help="config file path",
        type=str,
        default="./akconf.py",  # temporary
        required=False
    )

    args_dict = vars(parser.parse_args())
    if len(args_dict) == 0:
        parser.print_help(sys.stderr)
        sys.exit(1)

    return _akconf_parse(args_dict["conf_path"])


def _akconf_parse(conf_path: str) -> str:

    akconf: Any = SourceFileLoader("akconf", path.abspath(conf_path)).load_module()  # type: ignore
    return akconf.AKCONF.to_json()
