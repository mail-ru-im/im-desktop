if __name__ != '__main__':
	sys.exit(0)

import os.path, time, sys, glob, re

basepath = os.path.dirname(__file__)
file_name = os.path.abspath(os.path.join(basepath, sys.argv[1]))
f = open(file_name,'r')
a = [' type="unfinished"']
lst = []
for line in f:
	for word in a:
		if word in line:
			line = line.replace(word,'')
	lst.append(line)
f.close()
f = open(file_name,'w')
for line in lst:
	f.write(line)
f.close()

sys.exit(0)