import os, sys, collections.abc

def get_file_names(file_paths: collections.abc.Iterable[str]) -> list[str]:
	return [os.path.basename(file) for file in file_paths]


def strip_file_extension(file_name: str) -> str:
	return os.path.splitext(file_name)[0]


def strip_file_extensions(file_names: collections.abc.Iterable[str]) -> list[str]:
	return [strip_file_extension(file) for file in file_names]


def get_file_extension(file_name: str) -> str:
	return os.path.splitext(file_name)[1]


def get_file_extensions(file_names: collections.abc.Iterable[str]) -> list[str]:
	return [get_file_extension(file) for file in file_names]


def find_with_filter(folder_path: str, filter: collections.abc.Callable[[str], bool], recursive = True, sort = True) -> list[str]:
	files = []
	for file in os.listdir(folder_path):
		file_path = os.path.join(folder_path, file)
		if os.path.isdir(file_path):
			if recursive:
				files += find_with_filter(file_path, filter, True)
		else:
			if filter(file_path):
				files.append(file_path)
	if sort:
		files.sort()
	return files


def find_by_suffix(folder_path: str, file_suffixes: collections.abc.Iterable[str], recursive = True, sort = True) -> list[str]:
	def filter(file_path):
		for extension in file_suffixes:
			if file_path.endswith(extension):
				return True
		return False
	return find_with_filter(folder_path, filter, recursive, sort)


def find_by_extension(folder_path: str, file_extensions: collections.abc.Set[str], recursive = True, sort = True) -> list[str]:
	extensions = []
	for extension in file_extensions:
		extensions.append(extension if (len(extension) > 0 and extension[0] == '.') else ('.' + extension))
	def filter(file_path):
		return get_file_extension(file_path) in extensions
	return find_with_filter(folder_path, filter, recursive, sort)

def update_text_file(file_path: str, new_content: str) -> None:
	dirname = os.path.dirname(file_path)
	if (len(dirname) > 0) and (not os.path.isdir(dirname)):
		os.makedirs(dirname)
	try:
		with open(file_path, 'r') as file:
			text = file.read()
			if text == new_content:
				return
	except:
		pass
	with open(file_path, 'w') as file:
		file.write(new_content)


def create_dir_if_does_not_exist(directory_path: str) -> None:
	if not os.path.isdir(directory_path):
		os.makedirs(directory_path)


if __name__ == "__main__":
	for file in find_by_extension("." if len(sys.argv) <= 1 else sys.argv[1], [""] if len(sys.argv) <= 2 else sys.argv[2:]):
		print(file)
