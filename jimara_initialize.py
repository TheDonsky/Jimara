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
		make_symlinc("__Source__/Jimara", "Project/Windows/MSVS2019/Jimara/__SRC__")
		make_symlinc("__Source__/Jimara-Tests", "Project/Windows/MSVS2019/Jimara-Test/__SRC__")

		make_symlinc("Jimara-BuiltInAssets/", "Project/Windows/MSVS2019/Jimara-Test/Assets")
		make_symlinc("Jimara-BuiltInAssets/", "Project/Windows/MSVS2019/Jimara-Test/Assets")
		
		make_dir("Project/Windows/MSVS2019/Jimara-Test/Shaders")

		make_dir("__BUILD__")	
		make_dir("__BUILD__/MSVS2019")
		make_dir("__BUILD__/MSVS2019/Test")
		
		make_dir("__BUILD__/MSVS2019/Test/x64")
		make_dir("__BUILD__/MSVS2019/Test/x64/Debug")
		make_symlinc("Jimara-BuiltInAssets/", "__BUILD__/MSVS2019/Test/x64/Debug/Assets")
		make_symlinc("Project/Windows/MSVS2019/Jimara-Test/Shaders", "__BUILD__/MSVS2019/Test/x64/Debug/Shaders")
		make_dir("__BUILD__/MSVS2019/Test/x64/Release")
		make_symlinc("Jimara-BuiltInAssets/", "__BUILD__/MSVS2019/Test/x64/Release/Assets")
		make_symlinc("Project/Windows/MSVS2019/Jimara-Test/Shaders", "__BUILD__/MSVS2019/Test/x64/Release/Shaders")

		make_dir("__BUILD__/MSVS2019/Test/Win32")
		make_dir("__BUILD__/MSVS2019/Test/Win32/Debug")
		make_symlinc("Jimara-BuiltInAssets/", "__BUILD__/MSVS2019/Test/Win32/Debug/Assets")
		make_symlinc("Project/Windows/MSVS2019/Jimara-Test/Shaders", "__BUILD__/MSVS2019/Test/Win32/Debug/Shaders")
		make_dir("__BUILD__/MSVS2019/Test/Win32/Release")
		make_symlinc("Jimara-BuiltInAssets/", "__BUILD__/MSVS2019/Test/Win32/Release/Assets")
		make_symlinc("Project/Windows/MSVS2019/Jimara-Test/Shaders", "__BUILD__/MSVS2019/Test/Win32/Release/Shaders")
	
	elif os_info.os == os_linux:
		os.system("mkdir __BUILD__")

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
			os.system("sudo apt install libx11-dev")
			os.system("sudo apt install libx11-xcb-dev libxxf86vm-dev libxi-dev")

			os.system("sudo apt install python clang freeglut3-dev")
			
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
	if os_info.os != os_windows or os_info.admin:
		try:
			jimara_initialize()
			print ("Jimara initialized successfully!")
		except:
			print ("Error: Jimara not initialized...")
		if os_info.os == os_windows:
			os.system("pause")
	else:
		ctypes.windll.shell32.ShellExecuteW(None, "runas", sys.executable, __file__, None, 1)
