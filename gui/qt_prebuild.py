if __name__ != '__main__':
    sys.exit(0)


import os
import sys
import glob
import re
import subprocess
import pathlib


def get_dependencies_root():
    current_file_dir = pathlib.Path(__file__).parent.absolute()
    deps_root_dir = pathlib.Path.joinpath(current_file_dir,
                                          "..",
                                          "external_deps")
    return deps_root_dir

qt_folder_name = None
for folder in os.listdir(get_dependencies_root()):
    if folder.startswith("qt_") and not folder.startswith("qt_utils"):
        qt_folder_name = pathlib.Path(folder)
if not qt_folder_name:
    print(f"ERROR: Can't extract qt_folder_name {get_dependencies_root()}")
    sys.exit(1)
qt_path = pathlib.Path.joinpath(get_dependencies_root(), qt_folder_name, "bin")
print(f"-> qt_path = {qt_path}")


def file_contains_regex(path, regexes):
    assert os.path.exists(path)
    assert len(regexes) > 0

    def test_line(line):
        for regex in regexes:
            if re.search(regex, line):
                return True
        return False

    def test_lines(file):
        for line in file:
            if test_line(line):
                return True
        return False

    with open(path) as file:
        return test_lines(file)


META_REGEXES = ("Q_(OBJECT|SLOTS|SIGNALS)",)


def file_contains_qmeta(path):
    return file_contains_regex(path, META_REGEXES)


def build_mocs_in(dir):
    assert dir[-1] != '/'
    assert dir[-1] != '\\'

    files = glob.glob(dir + "/*.h")
    for file in files:
        if not file_contains_regex(file, META_REGEXES):
            continue

        file_name = os.path.splitext(file)[0]
        h_file = os.path.abspath(file_name + ".h").replace("\\", "/")
        moc_file = os.path.abspath(dir + "/moc_" + os.path.basename(file_name) + ".cpp").replace("\\", "/")

        if os.path.exists(moc_file):
            if os.path.getmtime(h_file) > os.path.getmtime(moc_file):
                print(os.path.basename(h_file), "is newer that", os.path.basename(moc_file), ">> rebuild")
                subprocess.call(str(qt_path) + "\\moc.exe " + '"' + h_file + '"' + " -b stdafx.h" + " -o " + '"' + moc_file + '"')
        else:
            print("build", os.path.basename(moc_file))
            subprocess.call(str(qt_path) + "\\moc.exe " + '"' + h_file + '"' + " -b stdafx.h" + " -o " + '"' + moc_file + '"')

    files = glob.glob(dir + "/moc_*.cpp")
    for file in files:
        file_size = os.path.getsize(file)
        MIN_SIZE = 3
        if file_size < MIN_SIZE:
            os.remove(file)

    dirs = glob.glob(dir + "/*/")
    for dir in dirs:
        build_mocs_in(dir[:-1])


def compile_ui_in(dir):
    assert dir[-1] != '/'
    assert dir[-1] != '\\'

    files = glob.glob(dir + "\*.ui")
    for file in files:
        file_name = os.path.splitext(file)[0]
        h_file = os.path.abspath(file_name + ".h").replace("\\", "/")
        ui_file = os.path.abspath(file_name + ".ui").replace("\\", "/")
        if os.path.exists(h_file):
            if os.path.getmtime(ui_file) > os.path.getmtime(h_file):
                print(os.path.basename(ui_file), "is newer that", os.path.basename(h_file), ">> rebuild")
                subprocess.call(str(qt_path) + "\\uic.exe " + '"' + ui_file + '"' + " -o " + '"' + h_file + '"')
        else:
            print("build", h_file)
            subprocess.call(str(qt_path) + "\\uic.exe " + '"' + ui_file + '"' + " -o " + '"' + h_file + '"')

    dirs = glob.glob(dir + "/*/")
    for dir in dirs:
        compile_ui_in(dir[:-1])

#
# Main
#


files = glob.glob("translations\*ts")
for file in files:
    file_name = os.path.splitext(file)[0]
    ts_file = os.path.abspath(file_name + ".ts").replace("\\", "/")
    qm_file = os.path.abspath(file_name + ".qm").replace("\\", "/")
    if os.path.exists(qm_file):
        if os.path.getmtime(ts_file) > os.path.getmtime(qm_file):
            print(os.path.basename(ts_file), "is newer that", os.path.basename(qm_file), ">> rebuild")
            subprocess.call(str(qt_path) + "\\lrelease.exe " + '"' + ts_file + '"')
    else:
        print("build", os.path.basename(qm_file))
        subprocess.call(str(qt_path) + "\\lrelease.exe " + '"' + ts_file + '"')

subprocess.call(str(qt_path) + "\\rcc.exe " + '"' + os.path.abspath("resource.qrc").replace("\\", "/") + '"' + " -o " + '"' + os.path.abspath("qresource").replace("\\", "/") + '"' + " --binary")

compile_ui_in(".")
build_mocs_in(".")

sys.exit(0)
