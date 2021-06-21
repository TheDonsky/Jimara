import jimara_initialize, os, sys

def compile_shader(file_path, output_dir = None):
	if output_dir is None:
		output_path = "\"" + file_path + ".spv\""
	else:
		if not os.path.isdir(output_dir):
			os.mkdir(output_dir)
		output_path = os.path.join(output_dir, os.path.basename(file_path) + ".spv")
	print ("Compiling \"" + file_path + "\" -> " + output_path)
	if jimara_initialize.os_info.os == jimara_initialize.os_windows:
		rv = os.system("%VULKAN_SDK%\\Bin32\\glslc.exe \"" + file_path + "\" -o \"" + output_path + "\"")
	elif jimara_initialize.os_info.os == jimara_initialize.os_linux:
		rv = os.system("glslc \"" + file_path + "\" -o \"" + output_path + "\"")
	else:
		rv = "OS not supported" 
	if rv != 0:
		print ("Error: " + str(rv))

def compile_shaders_in_folder(folder_path, recursive = True, extensions = [".vert", ".frag"], output_dir = None):
	for file in os.listdir(folder_path):
		file_path = os.path.join(folder_path, file)
		if os.path.isdir(file_path):
			if recursive:
				compile_shaders_in_folder(file_path, recursive, extensions, output_dir)
		else:
			important = False
			for extension in extensions:
				if file.endswith(extension):
					important = True
					break
			if important:
				compile_shader(file_path, output_dir)

if __name__ == "__main__":
	compile_shaders_in_folder("." if len(sys.argv) < 2 else sys.argv[1], output_dir = None if len(sys.argv) < 3 else sys.argv[2])
