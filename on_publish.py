from typing import TypeVar, cast
from distutils.cmd import Command
from distutils.dir_util import remove_tree, mkpath
import os
from os import path

_T = TypeVar('_T', bound=Command)


SDIST_TEMP = 'sdist_temp'
CLI_DIRNAME = 'akashi_cli'
BIN_NAME = 'akashi_renderer'
ENCODER_BIN_NAME = 'akashi_encoder'
KERNEL_BIN_NAME = 'akashi_kernel'


def _get_package_name(sdist_cmd: _T) -> str:
    return sdist_cmd.distribution.get_name().replace('-', '_')  # type: ignore


def _exec_cmake_build(sdist_cmd: _T):
    mkpath(SDIST_TEMP)

    cmake_build_dir = path.join(SDIST_TEMP, 'cmake_build')
    mkpath(cmake_build_dir)

    cmake_args = [
        '-DCMAKE_BUILD_TYPE=Release',
        '-DAKASHI_BUILD_TESTS=OFF',
    ]

    if 'CUSTOM_BOOST_TAR_PATH' in os.environ.keys():
        cmake_args.append('-DAKASHI_BOOST_TAR_PATH=' + os.environ['CUSTOM_BOOST_TAR_PATH'])

    build_concurrency = 4 if os.cpu_count() is None else cast(int, os.cpu_count()) * 2
    build_args = [
        '--', f'-j{build_concurrency}'
    ]

    sdist_cmd.spawn([
        'cmake',
        '-S', 'akashi_engine',
        '-B', cmake_build_dir
    ] + cmake_args)

    sdist_cmd.spawn(['cmake', '--build', cmake_build_dir] + build_args)

    sdist_cmd.copy_file(path.join(cmake_build_dir, BIN_NAME), SDIST_TEMP)
    sdist_cmd.copy_file(path.join(cmake_build_dir, ENCODER_BIN_NAME), SDIST_TEMP)
    sdist_cmd.copy_file(path.join(cmake_build_dir, KERNEL_BIN_NAME), SDIST_TEMP)

    sdist_credits_dir = path.join(SDIST_TEMP, 'credits')
    mkpath(sdist_credits_dir)
    sdist_cmd.copy_tree(path.join('akashi_engine', 'credits'), sdist_credits_dir)
    sdist_cmd.copy_tree(path.join(SDIST_TEMP, 'vendor', 'credits'), sdist_credits_dir)

    sdist_lib_dir = path.join(SDIST_TEMP, 'lib')
    mkpath(sdist_lib_dir)
    sdist_cmd.copy_tree(path.join(SDIST_TEMP, 'vendor', 'lib'), sdist_lib_dir)

    remove_tree(cmake_build_dir)
    remove_tree(path.join(SDIST_TEMP, "vendor"))
    remove_tree(path.join(SDIST_TEMP, "shared_temp"))


def pre_sdist(sdist_cmd: _T) -> None:
    _exec_cmake_build(sdist_cmd)


def post_sdist(sdist_cmd: _T) -> None:
    # remove egg-info
    remove_tree(_get_package_name(sdist_cmd) + '.egg-info')
