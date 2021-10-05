#!/usr/bin/env python3

import os
import sys
import platform
import pathlib
import subprocess
import logging
logging.basicConfig(format="%(levelname)-4s %(message)s",
                    level=logging.INFO,
                    datefmt="%Y-%m-%d %H:%M:%S")
log = logging.getLogger(__name__)


def get_platform():
    """ Darwin, Windows, etc """
    return platform.system()


def is_windows() -> bool:
    """ Is it Windows?
    :rtype: bool
    """
    return get_platform() == "Windows"


def is_macos() -> bool:
    """ Is it macOS?
    :rtype: bool
    """
    return get_platform() == "Darwin"


def is_linux() -> bool:
    """ Is it Linux?
    :rtype: bool
    """
    return get_platform() == "Linux"


def get_downloads_root() -> str:
    """ Return absolute path from relative path to im.desktop/downloads
    :rtype: str
    """
    current_file_dir = pathlib.Path(__file__).parent.absolute()
    downloads_root_dir = pathlib.Path.joinpath(current_file_dir,
                                               "..",
                                               "..",
                                               "downloads")
    return downloads_root_dir


def get_dependencies_root() -> str:
    """ Return absolute path from relative path to im.desktop/external_deps
    :rtype: str
    """
    current_file_dir = pathlib.Path(__file__).parent.absolute()
    deps_root_dir = pathlib.Path.joinpath(current_file_dir,
                                          "..",
                                          "..",
                                          "external_deps")
    return deps_root_dir


def get_qt_root_path() -> pathlib.Path:
    """ Return absolute path to Qt located on a disk
    :rtype: pathlib.Path
    """
    detected_qt_folder_name = None
    for folder in os.listdir(get_dependencies_root()):
        if folder.startswith("qt_") and not folder.startswith("qt_utils"):
            detected_qt_folder_name = pathlib.Path(folder)

    if not detected_qt_folder_name:
        log.error("")
        log.error(f"Can't detect detected_qt_folder_name {get_dependencies_root()}")
        sys.exit(1)

    return pathlib.Path.joinpath(get_dependencies_root(), detected_qt_folder_name)


def get_qt_lupdate_path() -> str:
    """ Return absolute path to Qt lupdate|lupdate.exe
    :rtype: str
    """
    if is_windows():
        return pathlib.Path.joinpath(get_downloads_root(), "bins", "lupdate.exe")

    if is_macos() or is_linux():
        qt_bin_path = pathlib.Path.joinpath(get_qt_root_path(), "bin")
        return pathlib.Path.joinpath(qt_bin_path, "lupdate")

    log.error("Can't found lupdate")
    return "Unknown_LUPDATE_path"


if __name__ == "__main__":

    lupdate_bin = get_qt_lupdate_path()
    log.info("")
    log.info(f"-> {pathlib.Path(__file__).name}")
    log.info("")
    log.info("[lupdate] Update translations...")
    log.info(f"[lupdate] {lupdate_bin}")

    languages = ("ru",)
    for lang in languages:
        lupdate_cmd = (str(lupdate_bin),
                       str(pathlib.Path("..")),
                       str(pathlib.Path.joinpath(pathlib.Path(".."), pathlib.Path(".."), "gui.shared")),
                       "-ts",
                       f"{lang}.ts",
                       "-noobsolete",
                       "-source-language",
                       "en",
                       "-target-language",
                       lang,
                       "-locations",
                       "none")
        lupdate_cmd_printable = " ".join(str(char) for char in lupdate_cmd)
        log.info(f"[lupdate] [{lang}] processing")
        log.info(f"[lupdate] [{lang}] -> {lupdate_cmd_printable}")

        lupdate_run = subprocess.run(lupdate_cmd, check=True)
        if not lupdate_run.returncode == 0:
            log.error("[lupdate]")
            log.error("[lupdate] LUpdate failed")
            log.error("[lupdate]")
            sys.exit(1)
        log.info(f"[lupdate] [{lang}] Done!")
