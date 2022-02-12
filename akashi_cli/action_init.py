from .parser import ParsedOption
from .utils import from_relpath

from os import path


CONFIG_FNAME = 'akconfig.py'
MAIN_FNAME = 'main.py'


def file_exists(fpath: str):

    if path.exists(fpath):
        raise Exception(f'akashi-init: error: `{fpath}` already exists in the current directory')


def copy_file(src_fpath: str, dst_fpath: str):

    with open(dst_fpath, 'w') as f:
        with open(src_fpath, 'r') as rf:
            f.write(rf.read())


def do_init(option: ParsedOption) -> None:

    try:
        file_exists(CONFIG_FNAME)
        file_exists(MAIN_FNAME)
    except Exception as e:
        print(e)
        return

    config_tpl_path = from_relpath(__file__, 'action_init_config_template.py')
    copy_file(config_tpl_path, CONFIG_FNAME)

    main_tpl_path = from_relpath(__file__, 'action_init_main_template.py')
    copy_file(main_tpl_path, MAIN_FNAME)

    print('Successfully created a new project. To test, run `akashi run akconfig.py`')
