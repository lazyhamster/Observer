import argparse
import os
import shutil
import sys
import uuid

INDENT = 4 * ' '

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("module_name")
    args = parser.parse_args()

    lc_module_name = args.module_name.lower()
    uc_module_name = args.module_name.upper()

    template_dir_name = "module_template"
    if template_dir_name == lc_module_name:
        return error("New module name conflicts with template directory name")

    working_dir_path = os.getcwd()
    template_dir_path = os.path.join(working_dir_path, template_dir_name)
    if not os.path.isdir(template_dir_path):
        return error("Template directory is missing, can not continue")
    for item in os.listdir(template_dir_path):
        if os.path.isdir(os.path.join(template_dir_path, item)):
            return error("There should not be a directory in the template")

    module_dir_path = os.path.join(working_dir_path, lc_module_name)
    if os.path.isdir(module_dir_path):
        return error('Module directory already exists')

    print(f"Template directory: [{template_dir_path}]")
    print(f"New module name: [{lc_module_name}]")
    print(f"New module directory: [{module_dir_path}]")
    print("")

    print("Copying template directory")
    shutil.copytree(template_dir_path, module_dir_path)

    uuid_object = uuid.uuid4()
    h = uuid_object.hex
    uuid_head = f"0x{h[0:8]}, 0x{h[8:12]}, 0x{h[12:16]}"
    uuid_tail = ", ".join(f"0x{h[i:i+2]}" for i in range(16, 32, 2))

    replacements = {
        r"%LOWERCASE_MODULE_NAME%": lc_module_name,
        r"%UPPERCASE_MODULE_NAME%": uc_module_name,
        r"%GUID_STRING%": f"{{{str(uuid_object).upper()}}}",
        r"%GUID_STRUCT%": f"{{ {uuid_head}, {{ {uuid_tail} }} }}",
    }

    def apply_replacements(old_string):
        new_string = old_string
        for variable, value in replacements.items():
            new_string = new_string.replace(variable, value)
        return new_string

    print("Processing module files")
    for old_file_name in os.listdir(module_dir_path):
        old_file_path = os.path.join(module_dir_path, old_file_name)
        new_file_name = apply_replacements(old_file_name)
        new_file_path = os.path.join(module_dir_path, new_file_name)
        if new_file_name != old_file_name:
            print(f'Renaming [{old_file_name}] to [{new_file_name}]')
            os.rename(old_file_path, new_file_path)

        print(f"Processing [{new_file_name}]")
        with open(new_file_path, 'r') as file_object:
            old_file_content = file_object.read()
        new_file_content = apply_replacements(old_file_content)
        if new_file_content == old_file_content:
            print(INDENT + 'No changes')
        else:
            with open(new_file_path, 'w') as file_object:
                file_object.write(new_file_content)
            print(INDENT + 'Content replaced')

    return 0

def error(s):
    print(f"ERROR: {s}", file=sys.stderr)
    return 1

if __name__ == '__main__':
    sys.exit(main())
