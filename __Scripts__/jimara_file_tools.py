import os, sys

def find_by_suffix(folder_path, file_extensions, recursive = True):
	files = []
	for file in os.listdir(folder_path):
		file_path = os.path.join(folder_path, file)
		if os.path.isdir(file_path):
			if recursive:
				files += find_by_suffix(file_path, file_extensions, True)
		else:
			for extension in file_extensions:
				if file.endswith(extension):
					files.append(file_path)
					break
	return files

def get_file_names(file_paths):
	return [os.path.basename(file) for file in file_paths]

def strip_file_extensions(file_names):
	return [os.path.splitext(file)[0] for file in file_names]

def get_file_extensions(file_names):
	return [os.path.splitext(file)[1] for file in file_names]

if __name__ == "__main__":
	for file in find_by_suffix("." if len(sys.argv) <= 1 else sys.argv[1], [""] if len(sys.argv) <= 2 else sys.argv[2:]):
		print(file)
