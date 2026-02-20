import ctypes, os

os_windows = 0
os_linux = 1
os_macos = 2

class os_information:
	def __init__(self):
		try:
			self.admin = os.getuid() == 0
			uname = os.uname().sysname.lower()
			if uname == "darwin":
				self.os = os_macos
			else:
				self.os = os_linux
		except AttributeError:
			self.admin = ctypes.windll.shell32.IsUserAnAdmin() != 0
			self.os = os_windows

os_info = os_information()


if __name__ == "__main__":
	print("OS: " + ("Windows" if os_info.os == os_windows else "Linux" if os_info.os == os_linux else "MacOS" if os_info.os == os_macos else "Unknown") + "; admin:" + str(os_info.admin))
