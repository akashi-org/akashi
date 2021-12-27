from typing import TypeVar
from distutils.command.install import install
from distutils.dir_util import mkpath
from os import path

BIN_NAME = 'akashi_renderer'
ENCODER_BIN_NAME = 'akashi_encoder'
KERNEL_BIN_NAME = 'akashi_kernel'
SDIST_TEMP = 'sdist_temp'
LIBAKPROBE_NAME = 'libakprobe.so'


def _get_install_lib_prefix(install_cmd: install, pkg_name: str) -> str:
    if install_cmd.install_lib is None:
        raise Exception('install_lib is None')
    return path.join(install_cmd.install_lib, pkg_name)


def _exec_install(install_cmd: install):

    akashi_cli_path = _get_install_lib_prefix(install_cmd, 'akashi_cli')

    inst_credits_dir = path.join(akashi_cli_path, 'credits')
    mkpath(inst_credits_dir)
    install_cmd.copy_tree(path.join(SDIST_TEMP, 'credits'), inst_credits_dir)
    install_cmd.copy_file(path.join(SDIST_TEMP, BIN_NAME), akashi_cli_path)
    install_cmd.copy_file(path.join(SDIST_TEMP, ENCODER_BIN_NAME), akashi_cli_path)
    install_cmd.copy_file(path.join(SDIST_TEMP, KERNEL_BIN_NAME), akashi_cli_path)

    inst_lib_dir = path.join(akashi_cli_path, 'lib')
    mkpath(inst_lib_dir)
    install_cmd.copy_tree(path.join(SDIST_TEMP, 'lib'), inst_lib_dir)

    install_cmd.copy_file(path.join(SDIST_TEMP, LIBAKPROBE_NAME), _get_install_lib_prefix(install_cmd, 'akashi_core'))


def post_install(install_cmd: install) -> None:
    _exec_install(install_cmd)
