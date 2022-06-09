import ctypes, sys, os, platform

os_windows = 0
os_linux = 1

class os_information:
	def __init__(self):
		try:
			self.admin = os.getuid() == 0
			self.os = os_linux
		except AttributeError:
			self.admin = ctypes.windll.shell32.IsUserAnAdmin() != 0
			self.os = os_windows

os_info = os_information()

def make_symlinc(folder_path, link_path):
	try:
		os.remove(link_path)
	except:
		pass
	try:
		os.symlink(os.path.relpath(folder_path, os.path.dirname(link_path)), link_path, True)
	except Exception as e:
		print ("Error: failed to create symbolic link (src:<" + folder_path + "> dst:<" + link_path + ">")
		raise e

def make_dir(dirname):
	if not os.path.isdir(dirname):
		os.makedirs(dirname)

def jimara_initialize():
	if os_info.os == os_windows:
		make_dir("__BUILD__/MSVS2019/Jimara")
		make_dir("__BUILD__/MSVS2019/Jimara-Editor")
		make_dir("__BUILD__/MSVS2019/Jimara-Test")

		make_symlinc("__Source__/Jimara", "Project/Windows/MSVS2019/Jimara/__SRC__")
		make_symlinc("__Source__/Jimara-Editor", "Project/Windows/MSVS2019/Jimara-Editor/__SRC__")
		make_symlinc("__Source__/Jimara-Tests", "Project/Windows/MSVS2019/Jimara-Test/__SRC__")
		
		def link_built_in_assets(build_path):
			platforms = ["/x64", "/Win32"]
			configs = ["/Debug", "/Release"]
			for plat in platforms:
				for conf in configs:
					dir_path = build_path + plat + conf
					make_dir(dir_path)
					make_symlinc("Jimara-BuiltInAssets/", dir_path + "/Assets")
		link_built_in_assets("__BUILD__/MSVS2019/Jimara-Editor")
		link_built_in_assets("__BUILD__/MSVS2019/Jimara-Test")
	
	elif os_info.os == os_linux:
		if os.system("rpm --version") != 0:
			os.system("sudo apt-get install libgtest-dev")
			os.system("sudo apt-get install cmake")
			os.system(
				"cd /usr/src/gtest\n" +
				"sudo cmake CMakeLists.txt GTEST_ROOT\n" +
				"sudo make\n")
			
			os.system("sudo apt install vulkan-tools")
			os.system("sudo apt install libvulkan-dev")
			os.system("sudo apt install vulkan-validationlayers-dev spirv-tools")
			
			os.system("sudo apt install libglfw3-dev")
			os.system("sudo apt install libglm-dev")
			os.system("sudo apt install zlib1g-dev")
			os.system("sudo apt install libx11-dev")
			os.system("sudo apt install libx11-xcb-dev libxxf86vm-dev libxi-dev")

			os.system("sudo apt install libfreetype-dev")

			os.system("sudo apt install clang freeglut3-dev")
			
			os.system("sudo apt install libopenal-dev")
		else:
			os.system("sudo dnf install gtest-devel")
			
			os.system("sudo dnf install vulkan-tools")
			os.system("sudo dnf install mesa-vulkan-devel")
			os.system("sudo dnf install vulkan-validation-layers-devel")
			os.system("sudo dnf install spirv-tools")
			os.system("sudo dnf install glslc")
			
			os.system("sudo dnf install glfw-devel")
			os.system("sudo dnf install glm-devel")
			os.system("sudo dnf install libXxf86vm-devel")


if __name__ == "__main__":
	no_elevate_present = "--no_elevate" in sys.argv
	no_pause_present = "--no_pause" in sys.argv
	if os_info.os != os_windows or os_info.admin or no_elevate_present:
		try:
			jimara_initialize()
			print ("Jimara initialized successfully!")
		except:
			print ("Error: Jimara not initialized...")
		if os_info.os == os_windows:
			if not no_pause_present:
				os.system("pause")
	else:
		ctypes.windll.shell32.ShellExecuteW(None, "runas", sys.executable, __file__, None, 1)
