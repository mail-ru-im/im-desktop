if __name__ != '__main__':
    sys.exit(0)


import os
import pathlib
import time
import sys
import glob
import re
import xml.etree.ElementTree


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



def build_mocs_in(dir):
    assert dir[-1] != '/'
    assert dir[-1] != '\\'

    files = glob.glob(dir + "/*h")
    for file in files:
        if not '/ui/' in os.path.abspath(file):
            f_name = os.path.splitext(file)[0]
            h_file = os.path.abspath(f_name + ".h").replace("\\", "/")
            moc_file = os.path.abspath(dir + "/moc_" + os.path.basename(f_name) + ".cpp").replace("\\", "/")
            if os.path.exists(moc_file):
                if os.path.getmtime(h_file) > os.path.getmtime(moc_file):
                    print(os.path.basename(h_file), "is newer that", os.path.basename(moc_file), ">> rebuild")
                    os.system('grep Q_OBJECT "' + h_file +'" -q && ' + str(qt_path) + "moc " + '"' + h_file + '"' + " -b stdafx.h" + " -o " + '"' + moc_file + '"')
            else:
                print("build", os.path.basename(moc_file))
                os.system('grep Q_OBJECT "' + h_file + '" -q && ' + str(qt_path) + "moc " + '"' + h_file + '"' + " -b stdafx.h" + " -o " + '"' + moc_file + '"')

    dirs = glob.glob(dir + "/*/")
    for _dir in dirs:
        build_mocs_in(_dir[:-1])


def compile_ui_in(dir):
    assert dir[-1] != '/'
    assert dir[-1] != '\\'

    files = glob.glob(dir + "/*.ui")
    for file in files:
        f_name = os.path.splitext(file)[0]
        h_file = os.path.abspath(f_name + ".h").replace("\\", "/")
        ui_file = os.path.abspath(f_name + ".ui").replace("\\", "/")
        if os.path.exists(h_file):
            if os.path.getmtime(ui_file) > os.path.getmtime(h_file):
                print(os.path.basename(ui_file), "is newer that", os.path.basename(h_file),">> rebuild")
                os.system(str(qt_path) + "uic " + '"' + ui_file + '"' + " -o " + '"' + h_file + '"')
        else:
            print("build", h_file)
            os.system(str(qt_path) + "uic " + '"' + ui_file + '"' + " -o " + '"' + h_file + '"')

    dirs = glob.glob(dir + "/*/")
    for _dir in dirs:
        compile_ui_in(_dir[:-1])

#
# Main
#

files = glob.glob("translations/*ts")
for file in files:
    file_name = os.path.splitext(file)[0]
    ts_file = os.path.abspath(file_name + ".ts").replace("\\", "/")
    qm_file = os.path.abspath(file_name + ".qm").replace("\\", "/")
    if os.path.exists(qm_file):
        if os.path.getmtime(ts_file) > os.path.getmtime(qm_file):
            print(os.path.basename(ts_file), "is newer that", os.path.basename(qm_file), ">> rebuild")
            os.system(str(qt_path) + "lrelease " + '"' + ts_file + '"')
    else:
        print("build", os.path.basename(qm_file))
        os.system(str(qt_path) + "lrelease " + '"' + ts_file + '"')


qrc_file = os.path.abspath("resource.qrc").replace("\\", "/")
qrc_cpp_file = os.path.abspath("qresource.cpp").replace("\\", "/")
if os.path.exists(qrc_cpp_file):
    qrc_cpp_time = os.path.getmtime(qrc_cpp_file)
    if os.path.getmtime(qrc_file) > qrc_cpp_time:
        print(os.path.basename(qrc_file), "is newer that", os.path.basename(qrc_cpp_file), ">> rebuild")
        os.system(str(qt_path) + "rcc " + '"' + qrc_file + '"' + " -o " + '"' + qrc_cpp_file + '"')
    else:
        changed = False
        e = xml.etree.ElementTree.parse(qrc_file).getroot()
        for child in e:
            for chil in child:
                file = os.path.abspath(chil.text).replace("\\", "/")
                if (os.path.getmtime(file) > qrc_cpp_time):
                    print(file + " is changed >> rebuild qrc")
                    os.system(str(qt_path) + "rcc " + '"' + qrc_file + '"' + " -o " + '"' + qrc_cpp_file + '"')
                    changed = True
                    break
            if changed:
                break
else:
    print("build", os.path.basename(qrc_cpp_file))
    os.system(str(qt_path) + "rcc " + '"' + qrc_file + '"' + " -o " + '"' + qrc_cpp_file + '"')

# compile_ui_in("./ui")

# build_mocs_in("widgets")
# build_mocs_in("logic")
# build_mocs_in("qt_supp")
build_mocs_in(".")
compile_ui_in(".")


if os.path.exists("moc_core_dispatcher.cpp"):
    if os.path.getmtime("core_dispatcher.h") > os.path.getmtime("moc_core_dispatcher.cpp"):
        print("core_dispatcher.h is newer that moc_core_dispatcher.cpp >> rebuild")
        os.system(str(qt_path) + "moc core_dispatcher.h -o moc_core_dispatcher.cpp -b stdafx.h")    
else:
    print("build moc_core_dispatcher.cpp")
    os.system(str(qt_path) + "moc " + '"' + os.path.abspath("core_dispatcher.h") + '"' + " -b stdafx.h"  + " -o " + '"' + os.path.abspath("moc_core_dispatcher.cpp") + '"')

sys.exit(0)
