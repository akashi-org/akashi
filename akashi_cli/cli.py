from .utils import (
    BIN_PATH,
    ENCODER_BIN_PATH,
    KERNEL_BIN_PATH,
    LIBRARY_PATH,
    ASSETS_DIR,
    libpython_path
)
from .parser import argument_parse, ParsedOption
from .action_init import do_init
from os import path

import signal
import threading
from subprocess import Popen
import os
import sys


class ServerThread(threading.Thread):

    def __init__(self, option: ParsedOption):
        super().__init__()
        self.daemon = True
        self.option = option
        self.proc = None

    def run(self):
        if self.option.action == 'run':
            self.__run_start()
        elif self.option.action == 'build':
            self.__build_start()
        elif self.option.action == 'kernel':
            self.__kernel_start()
        else:
            raise Exception(f'akashi: error: Invalid action `{self.option.action}` type found')

    def __run_start(self):
        self.proc = Popen(
            [BIN_PATH, self.option.akconf, self.option.conf_path],
            env=os.environ
        )
        self.proc.communicate()

    def __build_start(self):
        self.proc = Popen(
            [ENCODER_BIN_PATH, self.option.akconf, self.option.conf_path, self.option.out_fpath],
            env=os.environ
        )
        self.proc.communicate()

    def __kernel_start(self):
        self.proc = Popen(
            [KERNEL_BIN_PATH, self.option.akconf, BIN_PATH, self.option.conf_path, str(self.option.asp_port)],
            env=os.environ
        )
        self.proc.communicate()

    def close_and_wait(self):
        if self.proc:
            self.proc.terminate()
        self.join(timeout=3)
        if self.is_alive():
            print('ServerThread::close_and_wait(): Join timeout!', file=sys.stderr)


def akashi_cli() -> None:
    # [XXX] argument_parse() must be called before configuring signals, or weird bugs occur
    parsed_option = argument_parse()

    if parsed_option.action == 'init':
        return do_init(parsed_option)

    if 'LD_LIBRARY_PATH' in os.environ.keys():
        os.environ['LD_LIBRARY_PATH'] += os.pathsep + LIBRARY_PATH
    else:
        os.environ['LD_LIBRARY_PATH'] = LIBRARY_PATH

    if 'LD_PRELOAD' in os.environ.keys():
        os.environ['LD_PRELOAD'] += os.pathsep + libpython_path()
    else:
        os.environ['LD_PRELOAD'] = libpython_path()

    if 'QT_LOGGING_RULES' not in os.environ.keys():
        os.environ['QT_LOGGING_RULES'] = '*=false;*.critical=true'

    os.environ['QT_XCB_GL_INTEGRATION'] = 'xcb_egl'

    if 'AK_ASSET_DIR' not in os.environ.keys():
        os.environ['AK_ASSET_DIR'] = ASSETS_DIR

    os.environ['AK_CORE_ARGS'] = " ".join([path.abspath(parsed_option.conf_path)] + parsed_option.run_args)

    sigset: list[signal.Signals] = []
    sigset += [signal.SIGINT, signal.SIGHUP, signal.SIGQUIT, signal.SIGTERM]
    sigset += [signal.SIGPIPE, signal.SIGCHLD]

    signal.pthread_sigmask(signal.SIG_BLOCK, sigset)

    th_server = ServerThread(parsed_option)
    th_server.start()

    signal.sigwait(sigset)

    th_server.close_and_wait()
