import importlib.util
from typing import TYPE_CHECKING, Any
from os import path
import sys

if TYPE_CHECKING:
    from akashi_core.config import AKConf


def config_parse(conf_path: str) -> 'AKConf':

    mod_name = 'akconfig'
    spec = importlib.util.spec_from_file_location(mod_name, path.abspath(conf_path))
    if not spec or not spec.loader:
        raise Exception('akashi: error: spec not found')
    akconf = importlib.util.module_from_spec(spec)
    sys.modules[mod_name] = akconf
    spec.loader.exec_module(akconf)
    if not hasattr(akconf, '__akashi_export_config_fn'):
        raise Exception('akashi: error: No config function found. Perhaps you forget to add the export decorator?')
    return getattr(akconf, '__akashi_export_config_fn')()
