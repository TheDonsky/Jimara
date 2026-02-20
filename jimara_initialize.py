import ctypes, sys, os, platform

os_windows = 0
os_linux = 1
os_macos = 2

class os_information:
	def __init__(self):
		try:
			self.admin = os.getuid() == 0
			if platform.system().lower() == "darwin":
				self.os = os_macos
			else:
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


def fix_physx():
	cmake_lists_path = "Jimara-ThirdParty/NVIDIA/PhysX/PhysX/physx/source/compiler/cmake/linux/CMakeLists.txt"
	with open(cmake_lists_path, 'r') as fl:
		original_file_content = fl.read()
	lines = original_file_content.splitlines()
	lines[30] = "SET(CLANG_WARNINGS \"-ferror-limit=0 -Wall -Wextra -Werror -Wno-alloca -Wno-anon-enum-enum-conversion -Wstrict-aliasing=2 -Weverything -Wno-documentation-deprecated-sync -Wno-documentation-unknown-command -Wno-gnu-anonymous-struct -Wno-undef -Wno-unused-function -Wno-nested-anon-types -Wno-float-equal -Wno-padded -Wno-weak-vtables -Wno-cast-align -Wno-conversion -Wno-missing-noreturn -Wno-missing-variable-declarations -Wno-shift-sign-overflow -Wno-covered-switch-default -Wno-exit-time-destructors -Wno-global-constructors -Wno-missing-prototypes -Wno-unreachable-code -Wno-unused-macros -Wno-unused-member-function -Wno-used-but-marked-unused -Wno-weak-template-vtables -Wno-deprecated -Wno-non-virtual-dtor -Wno-invalid-noreturn -Wno-return-type-c-linkage -Wno-reserved-id-macro -Wno-c++98-compat-pedantic -Wno-unused-local-typedef -Wno-old-style-cast -Wno-newline-eof -Wno-unused-private-field -Wno-format-nonliteral -Wno-implicit-fallthrough -Wno-undefined-reinterpret-cast -Wno-disabled-macro-expansion -Wno-zero-as-null-pointer-constant -Wno-shadow -Wno-unknown-warning-option -Wno-atomic-implicit-seq-cst -Wno-extra-semi-stmt -Wno-suggest-override -Wno-suggest-destructor-override -Wno-dtor-name -Wno-reserved-identifier -Wno-bitwise-instead-of-logical -Wno-unused-but-set-variable -Wno-unaligned-access -Wno-unsafe-buffer-usage -Wno-switch-default -Wno-invalid-offsetof\")"
	lines[31] = "SET(GCC_WARNINGS \"-Wall -Werror -Wno-invalid-offsetof -Wno-uninitialized -Wno-suggest-override -Wno-suggest-destructor-override -Wno-dtor-name\")"
	new_file_content = ""
	for line in lines:
		new_file_content += line + '\n'
	with open(cmake_lists_path, 'w') as fl:
		fl.write(new_file_content)


def jimara_initialize():
	fix_physx()

	if os_info.os == os_windows:
		make_dir("__BUILD__/Windows/Jimara")
		make_dir("__BUILD__/Windows/Jimara-Test")

		make_symlinc("__Source__/Jimara", "Project/Windows/MSVS2019/Jimara/__SRC__")
		make_symlinc("__Source__/Jimara-Editor", "Project/Windows/MSVS2019/Jimara-Editor/__SRC__")
		make_symlinc("__Source__/Jimara-EditorExecutable", "Project/Windows/MSVS2019/Jimara-EditorExecutable/__SRC__")
		make_symlinc("__Source__/Jimara-StateMachines", "Project/Windows/MSVS2019/Jimara-StateMachines/__SRC__")
		make_symlinc("__Source__/Jimara-StateMachines-Editor", "Project/Windows/MSVS2019/Jimara-StateMachines-Editor/__SRC__")
		make_symlinc("__Source__/Jimara-GenericInputs", "Project/Windows/MSVS2019/Jimara-GenericInputs/__SRC__")
		make_symlinc("__Source__/Jimara-SampleGame", "Project/Windows/MSVS2019/Jimara-SampleGame/__SRC__")
		make_symlinc("__Source__/Jimara-Tests", "Project/Windows/MSVS2019/Jimara-Test/__SRC__")
		
		def link_built_in_assets(build_path):
			platforms = ["/x64", "/Win32"]
			configs = ["/Debug", "/Release"]
			for plat in platforms:
				for conf in configs:
					dir_path = build_path + plat + conf
					make_dir(dir_path)
					make_symlinc("Jimara-BuiltInAssets/", dir_path + "/Assets")
		link_built_in_assets("__BUILD__/Windows/Jimara")
		link_built_in_assets("__BUILD__/Windows/Jimara-Test")
	
	elif os_info.os == os_linux:
		# Anything with apt:
		if os.system("apt --version") == 0:
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


		# RPM-based:
		elif os.system("rpm --version") == 0:
			os.system("sudo dnf install gtest-devel")
			
			os.system("sudo dnf install vulkan-tools")
			os.system("sudo dnf install mesa-vulkan-devel")
			os.system("sudo dnf install vulkan-validation-layers-devel")
			os.system("sudo dnf install spirv-tools")
			os.system("sudo dnf install glslc")
			
			os.system("sudo dnf install glfw-devel")
			os.system("sudo dnf install glm-devel")
			os.system("sudo dnf install libXxf86vm-devel")


		# Anything with pacman:
		elif os.system("pacman --version") == 0:
			os.system("sudo pacman -S make")
			os.system("sudo pacman -S cmake")
			os.system("sudo pacman -S vulkan-tools")
			os.system("sudo pacman -S glm")
			os.system("sudo pacman -S glfw-x11")
			os.system("sudo pacman -S gtest")
			os.system("sudo pacman -S kdialog")
		

		# Unsupported:
		else:
			print("jimara_initialize - Unsupported desktop environment!")
			exit(1)

	elif os_info.os == os_macos:
		make_dir("__BUILD__/MacOS/Jimara")
		make_dir("__BUILD__/MacOS/Jimara-Test")

		make_symlinc("Jimara-BuiltInAssets/", "__BUILD__/MacOS/Jimara/Assets")
		make_symlinc("Jimara-BuiltInAssets/", "__BUILD__/MacOS/Jimara-Test/Assets")

		if os.system("brew --version >/dev/null 2>&1") == 0:
			os.system("brew install cmake ninja glfw molten-vk vulkan-headers vulkan-loader glslang spirv-tools openal-soft freetype")
		else:
			print("jimara_initialize - Homebrew not found; skipped dependency install on Macos")


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
