
import os;

def remove_comments(source: str) -> str:
    result = ""
    i = 0
    while i < len(source):
        if (i < (len(source) - 1)) and source[i] == '/' and source[i + 1] == '/':
            # Skip line if we have a comment starting with '//':
            while i < len(source) and source[i] != '\n':
                i += 1
            continue
        elif (i < (len(source) - 1)) and source[i] == '/' and source[i + 1] == '*':
            # Skip comment segment '/* ... */':
            i += 2
            while i < len(source) and (not (source[i - 1] == '*' and source[i] == '/')):
                # Keep new-line characters to avoid loosing line index information:
                if source[i] == '\n':
                    result += '\n'
                i += 1
        else:
            # Keep 'Out-of-comment' characters:
            result += source[i]
        i += 1
    return result


def get_abspath(filename: str, parent_path) -> str:
	path = filename
	if parent_path is not None:
		parent_folder = os.path.realpath(parent_path)
		if os.path.isfile(parent_folder):
			parent_folder = os.path.split(parent_folder)[0]
		path = os.path.join(parent_folder, filename)
	if not os.path.exists(path):
		return None
	return os.path.realpath(path)


class source_data:
	def __init__(self, path: str, content: str) -> None:
		self.path = path
		self.content = content if content is not None else ""
		self.code_lines = remove_comments(self.content).splitlines()

	def __str__(self) -> str:
		tab = "    "
		text = ("{\n" +
		  tab + "'" + self.path + "\':\n" +
		  tab + "___________________________________\n")
		for line in self.content.splitlines():
			text += tab + line + "\n"
		text += tab + "___________________________________\n}"
		return text


class source_cache:
	def __init__(self, include_dirs: list = []) -> None:
		self.include_dirs = [get_abspath(dir, None) for dir in include_dirs]
		self.file_data = {}

	def get_source_path(self, filename: str, including_source: str = None) -> str:
		if filename is None:
			return None
		if including_source is not None:
			including_source = self.get_source_path(including_source, None)
			result = get_abspath(filename, including_source)
			if result is not None and os.path.isfile(result):
				return result
		for include_dir in self.include_dirs:
			result = get_abspath(filename, include_dir)
			if result is not None and os.path.isfile(result):
				return result
		result = get_abspath(filename, None)
		if result is not None and os.path.isfile(result):
			return result
		return None
	
	def get_local_path(self, filename: str, including_source: str = None) -> str:
		filename = self.get_source_path(filename, including_source)
		for include_dir in self.include_dirs:
			if filename.startswith(include_dir):
				i = len(include_dir)
				while i < len(filename) and (filename[i] == '\\' or filename[i] == '/'):
					i += 1
				return os.path.join(os.path.basename(include_dir), filename[i:])
		return filename

	def get_content(self, filename: str, including_source: str = None) -> source_data:
		filename = self.get_source_path(filename, including_source)
		if filename is None:
			return None
		if filename in self.file_data:
			return self.file_data[filename]
		try:
			with open(filename, 'r') as file:
				content = file.read()
			self.file_data[filename] = source_data(filename, content)
			return source_data(filename, content)
		except:
			print("source_cache.get_content - Failed to read file '" + filename + "'!")
			return None


if __name__ == "__main__":
	cache = source_cache(["__Test__/include_dir_0", "__Test__/include_dir_1"])
	print(cache.get_content("subdir_a/file_a.h"))
	print(cache.get_content("file_a.h", "subdir_b/file_b.h"))
	print(cache.get_content("C:/Users/Donsky/Desktop/CodeAnalysis/include_dir_1/subdir_b/file_a.h"))
