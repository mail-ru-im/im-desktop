if __name__ != '__main__':
	sys.exit(0)

import os.path, time, sys, glob

files = glob.glob("translations/*ts")
for file in files:
	file_name = os.path.splitext(file)[0]
	ts_file = os.path.abspath(file_name + ".ts").replace("\\", "/")
	qm_file = os.path.abspath(file_name + ".qm").replace("\\", "/")
	if os.path.exists(qm_file):
		if os.path.getmtime(ts_file) > os.path.getmtime(qm_file):
			print(os.path.basename(ts_file), "is newer that", os.path.basename(qm_file),">> rebuild")
                        os.system("/home/rz/Qt/5.5/gcc_64/bin/lrelease " + '"' + ts_file + '"')
	else:
		print("build", os.path.basename(qm_file))
                os.system("/home/rz/Qt/5.5/gcc_64/bin/lrelease " + '"' + ts_file + '"')

os.system("/home/rz/Qt/5.5/gcc_64/bin/rcc " + '"' + os.path.abspath("resource.qrc").replace("\\", "/") + '"' + " -o " + '"' + os.path.abspath("qresource").replace("\\", "/") + '"' + " -binary")

sys.exit(0)
