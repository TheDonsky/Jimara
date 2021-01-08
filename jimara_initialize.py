import ctypes, sys, os, platform

jimara_os_windows = 0
jimara_os_linux = 1

class jimara_os_information:
	def __init__(self):
		try:
			self.admin = os.getuid() == 0
			self.os = jimara_os_linux
		except AttributeError:
			self.admin = ctypes.windll.shell32.IsUserAnAdmin() != 0
			self.os = jimara_os_windows

jimara_os_info = jimara_os_information()

def jimara_make_symlinc(folder_path, link_path):
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
	jimara_make_symlinc("__Source__", "Project/Windows/MSVS2019/__SRC__")
	jimara_make_symlinc("__Tests__", "Project/Windows/MSVS2019/Jimara-Test/__SRC__")
	

if __name__ == "__main__":
	if jimara_os_info.os != jimara_os_windows or jimara_os_info.admin:
		try:
			jimara_initialize()
			print ("Jimara initialized successfully!")
		except:
			print ("Error: Jimara not initialized...")
		if jimara_os_info.os == jimara_os_windows:
			os.system("pause")
	else:
		ctypes.windll.shell32.ShellExecuteW(None, "runas", sys.executable, __file__, None, 1)
