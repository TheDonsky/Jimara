import os, sys

def get_file_names(file_paths):
	return [os.path.basename(file) for file in file_paths]

def strip_file_extension(file_name):
	return os.path.splitext(file_name)[0]

def strip_file_extensions(file_names):
	return [strip_file_extension(file) for file in file_names]

def get_file_extension(file_name):
	return os.path.splitext(file_name)[1]

def get_file_extensions(file_names):
	return [get_file_extension(file) for file in file_names]
	
def find_with_filter(folder_path, filter, recursive = True):
	files = []
	for file in os.listdir(folder_path):
		file_path = os.path.join(folder_path, file)
		if os.path.isdir(file_path):
			if recursive:
				files += find_with_filter(file_path, filter, True)
		else:
			if filter(file_path):
				files.append(file_path)
	return files

def find_by_suffix(folder_path, file_suffixes, recursive = True):
	def filter(file_path):
		for extension in file_suffixes:
			if file_path.endswith(extension):
				return True
		return False
	return find_with_filter(folder_path, filter, recursive)

def find_by_extension(folder_path, file_extensions, recursive = True):
	extensions = []
	for extension in file_extensions:
		extensions.append(extension if (len(extension) > 0 and extension[0] == '.') else ('.' + extension))
	def filter(file_path):
		return get_file_extension(file_path) in extensions
	return find_with_filter(folder_path, filter, recursive)

if __name__ == "__main__":
	for file in find_by_extension("." if len(sys.argv) <= 1 else sys.argv[1], [""] if len(sys.argv) <= 2 else sys.argv[2:]):
		print(file)
