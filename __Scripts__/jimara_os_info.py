import ctypes, os

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


if __name__ == "__main__":
	print("OS: " + ("Windows" if os_info.os == os_windows else "Linux" if os_info.os == os_linux else "Unknown") + "; admin:" + str(os_info.admin))
