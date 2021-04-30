from typing import TypeVar
from distutils.command.install import install
from distutils.dir_util import mkpath
from os import path

_T = TypeVar('_T', bound=install)

BIN_NAME = 'akashi_renderer'
ENCODER_BIN_NAME = 'akashi_encoder'
KERNEL_BIN_NAME = 'akashi_kernel'
SDIST_TEMP = 'sdist_temp'


def _get_install_lib_prefix(install_cmd: _T) -> str:
    if install_cmd.install_lib is None:
        raise Exception('install_lib is None')
    return path.join(install_cmd.install_lib, 'akashi_cli')


def _exec_install(install_cmd: _T):

    install_prefix = _get_install_lib_prefix(install_cmd)

    inst_credits_dir = path.join(install_prefix, 'credits')
    mkpath(inst_credits_dir)
    install_cmd.copy_tree(path.join(SDIST_TEMP, 'credits'), inst_credits_dir)
    install_cmd.copy_file(path.join(SDIST_TEMP, BIN_NAME), install_prefix)
    install_cmd.copy_file(path.join(SDIST_TEMP, ENCODER_BIN_NAME), install_prefix)
    install_cmd.copy_file(path.join(SDIST_TEMP, KERNEL_BIN_NAME), install_prefix)

    inst_lib_dir = path.join(install_prefix, 'lib')
    mkpath(inst_lib_dir)
    install_cmd.copy_tree(path.join(SDIST_TEMP, 'lib'), inst_lib_dir)


def post_install(install_cmd: _T) -> None:
    _exec_install(install_cmd)
