from importlib.machinery import SourceFileLoader
from typing import TYPE_CHECKING, Any
from os import path

if TYPE_CHECKING:
    from akashi_core.config import AKConf


def config_parse(conf_path: str) -> 'AKConf':

    akconf: Any = SourceFileLoader("akconfig", path.abspath(conf_path)).load_module()  # type: ignore
    if not hasattr(akconf, '__akashi_export_config_fn'):
        raise Exception('akashi: error: No config function found. Perhaps you forget to add the export decorator?')
    return getattr(akconf, '__akashi_export_config_fn')()
