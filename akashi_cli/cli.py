from .utils import BIN_PATH, ENCODER_BIN_PATH, KERNEL_BIN_PATH, LIBRARY_PATH, libpython_path
from .parser import argument_parse, ParsedOption

import signal
import threading
from subprocess import Popen
import os


class ServerThread(threading.Thread):

    def __init__(self, option: ParsedOption):
        super().__init__()
        self.daemon = True
        self.action = option.action
        self.akconf = option.akconf
        self.conf_path = option.conf_path
        self.asp_port = str(option.asp_port)
        self.proc = None

    def run(self):

        if self.action == 'debug':
            self.__debug_run()
        elif self.action == 'build':
            self.__build_run()
        elif self.action == 'kernel':
            self.__kernel_run()
        else:
            raise Exception(f'invalid action `{self.action}`type found')

    def __debug_run(self):
        self.proc = Popen(
            [BIN_PATH, self.akconf, self.conf_path], env=os.environ
        )
        self.proc.communicate()

    def __build_run(self):
        self.proc = Popen(
            [ENCODER_BIN_PATH, self.akconf, self.conf_path], env=os.environ
        )
        self.proc.communicate()

    def __kernel_run(self):
        self.proc = Popen(
            [KERNEL_BIN_PATH, self.akconf, BIN_PATH, self.conf_path, self.asp_port], env=os.environ
        )
        self.proc.communicate()

    def terminate(self):
        if self.proc:
            self.proc.terminate()


def akashi_cli() -> None:
    # [XXX] argument_parse() must be called before configuring signals, or weird bugs occur
    parsed_option = argument_parse()

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

    sigset: list[signal.Signals] = []
    sigset += [signal.SIGINT, signal.SIGHUP, signal.SIGQUIT, signal.SIGTERM]
    sigset += [signal.SIGPIPE, signal.SIGCHLD]

    signal.pthread_sigmask(signal.SIG_BLOCK, sigset)

    th_server = ServerThread(parsed_option)
    th_server.start()

    signal.sigwait(sigset)

    th_server.terminate()
    print('')
