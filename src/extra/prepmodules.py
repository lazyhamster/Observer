import os;
import sys;
import shutil;

MNAME = '%MODNAME%'
MUNAME = '%UPMODNAME%'

def process_dir(path, modname):
	dirList = os.listdir(path)
	for name in dirList:
		# Replace module name in name
		if str.find(name, MNAME) != -1:
			newName = str.replace(name, MNAME, modname)
			print 'Renaming {0} -> {1}'.format(name, newName)
			shutil.move(os.path.join(path, name), os.path.join(path, newName))
			name = newName
			
		# Replace uppercased module name in name
		elif str.find(name, MUNAME) != -1:
			newName = str.replace(name, MUNAME, modname)
			print 'Renaming {0} -> {1}'.format(name, newName)
			shutil.move(os.path.join(path, name), os.path.join(path, newName))
			name = newName
			
		fullpath = os.path.join(path, name)
		
		if os.path.isdir(fullpath):
			print "Processing directory : ", name
			process_dir(fullpath, modname)
		
		elif os.path.isfile(fullpath) and (str.find(name, '.py') != len(name) - 3):
			print "Processing file : ", name
			f = open(fullpath, "r")
			content = f.read()
			f.close()
			if (str.find(content, MNAME) != -1) or (str.find(content, MUNAME) != -1):
				print "File content replaced"
				newContent = str.replace(content, MNAME, modname)
				newContent = str.replace(newContent, MUNAME, str.upper(modname))
				fNew = open(fullpath, "w")
				fNew.write(newContent)
				fNew.close()

argc = len(sys.argv);
if (argc != 3) and (argc != 2):
	print "Usage: prepmodules.py <module_name> <template_path>"
	sys.exit()

module_name = sys.argv[1]
template_dir = sys.argv[2] if (argc == 3) else os.getcwd()

if (not os.path.exists(template_dir)) or (not os.path.isdir(template_dir)):
	print "ERROR: Template dir does not exists"
	sys.exit()

print "New module name : [%s]" % module_name
print "Template dir : [%s]" % template_dir
print "\n"

process_dir(template_dir, module_name)
