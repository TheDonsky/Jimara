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


def jimara_initialize():
	make_symlinc("__Source__/Jimara", "Project/Windows/MSVS2019/Jimara/__SRC__")
	make_symlinc("__Source__/Jimara-Tests", "Project/Windows/MSVS2019/Jimara-Test/__SRC__")
	
	if os_info.os == os_linux:
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
