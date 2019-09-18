if __name__ != '__main__':
	sys.exit(0)

import os.path, time, sys, glob, re, argparse, subprocess

qt_path = os.path.dirname(os.path.abspath(__file__ + "/..")).replace("\\", "/")
qt_path += "/external/qt/windows/bin/"

parser = argparse.ArgumentParser()
parser.add_argument("-config", type=str, help="build configuration (release or debug)")
args = parser.parse_args()

files_to_pack = [
	"icq.exe",
	"libvoip_x86.dll",
	"D3DCompiler_43.dll"]

bundle_to_pack = ["switcher.exe"]


def get_7z_path():
	path_7z = os.path.dirname(os.path.abspath(__file__ + "/..")).replace("\\", "/")
	path_7z += "/external/platform_specific/windows/x86/7z/7z.exe"

	if not os.path.exists(path_7z):
		program_files = os.environ["ProgramFiles(x86)"]
		path_7z = program_files + "\\7-Zip\\7z.exe"

		if not os.path.exists(path_7z):
			program_files = os.environ["ProgramW6432"]
			path_7z = program_files + "\\7-Zip\\7z.exe"

	return path_7z

def pack(files_list, archive_name):
	archive_name = "resources/" + archive_name
	print("pack: " + ",\n".join(files_list) + " -> " + archive_name)

	sys.stdout.flush()
	path_7z = get_7z_path()
	if not os.path.exists(path_7z):
		print("pack: 7z not found in " + path_7z)
		sys.exit(-1)

	if (not os.path.exists(archive_name) or any(os.path.getmtime(x) > os.path.getmtime(archive_name) for x in files_list)):
		files_to_pack_str = ""

		for file_to_pack in files_list:
			if not os.path.exists(file_to_pack):
				print("pack: " + os.path.abspath(file_to_pack) + " not found")
				sys.exit(-1)

			files_to_pack_str += " " + file_to_pack

		if os.path.exists(archive_name):
			os.remove(archive_name)

		command = '"' + path_7z + '"' + " a -t7z -mx=9 -mf=off -r " + archive_name + files_to_pack_str
		print("pack: running " + command)
		sys.stdout.flush()
		ret = os.system(command)
		if (ret != 0):
			print("pack failed with exit code " + str(ret))
			sys.exit(ret)
	else:
		print(archive_name + " is up to date")

def pack_bundle():
	to_pack = ["resources/bundle/" + x for x in bundle_to_pack]
	pack(to_pack, "bundle.7z")

def pack_files():
	to_pack = ["../bin/" + args.config + "/" + x for x in files_to_pack]
	pack(to_pack, "files.7z")

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

	mocs_dir = os.path.abspath(args.config.lower() + "/mocs")
	if not os.path.exists(mocs_dir):
		os.makedirs(mocs_dir)

	files = glob.glob(dir + "/*.h")
	for file in files:
		if not file_contains_qmeta(file):
			continue

		file_name = os.path.splitext(file)[0]
		h_file = os.path.abspath(file_name + ".h").replace("\\", "/")
		moc_file = os.path.abspath(mocs_dir + "/moc_" + os.path.basename(file_name) + ".cpp").replace("\\", "/")

		if os.path.exists(moc_file):
			if os.path.getmtime(h_file) > os.path.getmtime(moc_file):
				print(os.path.basename(h_file), "is newer that", os.path.basename(moc_file),">> rebuild")
				subprocess.call(qt_path + "moc.exe " + '"' + h_file + '"' + " -b stdafx.h" + " -o " + '"' + moc_file + '"')
		else:
			print("build", os.path.basename(moc_file))
			subprocess.call(qt_path + "moc.exe " + '"' + h_file + '"' + " -b stdafx.h" + " -o " + '"' + moc_file + '"')

	files = glob.glob(dir + "/moc_*.cpp")
	for file in files:
		file_size = os.path.getsize(file)
		MIN_SIZE = 3
		if file_size < MIN_SIZE:
			os.remove(file)

	dirs = glob.glob(dir + "/*/")
	for dir in dirs:
		build_mocs_in(dir[:-1])

def compile_resources():
	qrc_file_name = os.path.abspath("resources/resource.qrc").replace("\\", "/")
	cpp_file_name = os.path.abspath("resources/resource.cpp").replace("\\", "/")
	qss_file_name = os.path.abspath("resources/styles/styles.qss").replace("\\", "/")

	if ((not os.path.exists(cpp_file_name)) or (os.path.getmtime(qrc_file_name) > os.path.getmtime(cpp_file_name)) or args.config.lower() != "debug" or (os.path.getmtime(qss_file_name) > os.path.getmtime(cpp_file_name))):
		subprocess.call(qt_path + "rcc.exe " + '"' + qrc_file_name + '"' + " -o " + '"' + cpp_file_name + '"')

if __name__ == "__main__":
	translation_changed = False

	files = glob.glob("translations\*ts")
	for file in files:
		file_name = os.path.splitext(file)[0]
		ts_file = os.path.abspath(file_name + ".ts").replace("\\", "/")
		qm_file = os.path.abspath(file_name + ".qm").replace("\\", "/")
		if os.path.exists(qm_file):
			if os.path.getmtime(ts_file) > os.path.getmtime(qm_file):
				translation_changed = True
				print(os.path.basename(ts_file), "is newer that", os.path.basename(qm_file),">> rebuild")
				subprocess.call(qt_path + "lrelease.exe " + '"' + ts_file + '"')
		else:
			print("build", os.path.basename(qm_file))
			subprocess.call(qt_path + "lrelease.exe " + '"' + ts_file + '"')
			translation_changed = True

	pack_files()
	#pack_bundle()
	build_mocs_in(".")
	build_mocs_in("../gui.shared")
	compile_resources()

	sys.exit(0)
