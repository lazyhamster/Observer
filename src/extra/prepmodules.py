import os;
import sys;
import shutil;
import zipfile;

MNAME = '%MODNAME%'
MUNAME = '%UPMODNAME%'

def replace_file_content(file_path, mod_name):
	print("Processing file : ", os.path.basename(file_path))
	f = open(file_path, "r")
	content = f.read()
	f.close()
	if (str.find(content, MNAME) != -1) or (str.find(content, MUNAME) != -1):
		print("    File content replaced")
		newContent = str.replace(content, MNAME, mod_name)
		newContent = str.replace(newContent, MUNAME, str.upper(mod_name))
		fNew = open(file_path, "w")
		fNew.write(newContent)
		fNew.close()
	else:
		print("    No changes")


argc = len(sys.argv);
if (argc != 2):
	print("Usage: prepmodules.py <module_name>")
	sys.exit()

module_name = sys.argv[1]
working_dir = os.getcwd()

template_arch = os.path.join(working_dir, 'mod_template.zip')
if ((not os.path.exists(template_arch)) or (not os.path.isfile(template_arch))):
	print("ERROR: Template data archive is missing. Can not continue")
	sys.exit()

print("New module name : [%s]" % module_name)
print("Module files location : [%s]" % working_dir)
print("\n")

print("Unpacking template data")
with zipfile.ZipFile(template_arch, "r") as zip_ref:
	zip_ref.extractall(working_dir)
	
module_data_path = os.path.join(working_dir, module_name)

print("Renaming module directory")
shutil.move(os.path.join(working_dir, MNAME), module_data_path)

print("Processing module files")
dir_list = os.listdir(module_data_path)
for name in dir_list:
	if str.find(name, MNAME) != -1:
		newName = str.replace(name, MNAME, module_name)
		print("Renaming ", name, " -> ", newName)
		shutil.move(os.path.join(module_data_path, name), os.path.join(module_data_path, newName))
		name = newName
			
	file_path = os.path.join(module_data_path, name)
	if os.path.isfile(file_path):
		replace_file_content(file_path, module_name)
	else:
		print("ERROR There should not be a directory in the modile data")
