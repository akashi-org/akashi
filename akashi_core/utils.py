import typing as tp
import sys
import re
import os
from .config import AKConf


def version_check() -> tuple[bool, str]:
    if '.pyenv/versions' not in sys.executable:
        return (True, '')

    sys_ver = sys.version.split(' ')[0]
    exec_ver = re.findall(r'versions/([^/]*)/bin', sys.executable)[0]

    if exec_ver != sys_ver:
        msg = f'''Found version mismatch!
--------------------------------------------------------------------------------------------
sys.version: {sys.version.replace(os.linesep, ' ')}
sys.executable: {sys.executable}

If you use pyenv, you can resolve this issue. Please see the documentation for more details.
--------------------------------------------------------------------------------------------'''
        return (False, msg)
    else:
        return (True, '')
