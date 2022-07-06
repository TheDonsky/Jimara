import json, os


class source_info:
	date_key = 'last_modified'
	dependencies_key = "dependencies"
	include_statement = "#include"

	def __init__(self, include_dirs : list, cache_path : str) -> None:
		self.__include_dirs = include_dirs.copy()
		self.__dirty_records = set()
		self.__cache_path = cache_path
		self.__data = {}
		self.__load_from_cache()

	def get_source_dependencies(self, file_path : str) -> set:
		abs_path = self.get_real_path(file_path, None)
		rv = set()
		if abs_path is None:
			print("source_info.get_file_dependencies - Failed to get absolute path of '" + file_path + "'!")
		else:
			self.__collect_file_dependencies(abs_path, rv)
		return rv

	def source_dirty(self, file_path) -> bool:
		abs_path = self.get_real_path(file_path, None)
		if abs_path is None:
			return True
		if abs_path in self.__dirty_records:
			return True
		dependencies = set()
		self.__collect_file_dependencies(abs_path, dependencies)
		if abs_path in self.__dirty_records:
			return True
		for dependency in dependencies:
			if dependency in self.__dirty_records:
				return True
		return False

	def update_cache(self) -> None:
		try:
			text = json.dumps(self.__data, indent = 4, separators= (',', ': '))
			with open(self.__cache_path, 'w') as file:
				file.write(text)
		except:
			print("source_info.update_cache - Failed to store data to '" + self.__cache_path + "'!")

	def get_real_path(self, dependency : str, source_path : str) -> str:
		try:
			if source_path is not None:
				source_dir = os.path.dirname(source_path)
				relative = os.path.join(source_dir, dependency)
				if os.path.isfile(relative):
					return os.path.realpath(relative)

			for include_dir in self.__include_dirs:
				relative = os.path.join(include_dir, dependency)
				if os.path.isfile(relative):
					return os.path.realpath(relative)

			if os.path.isfile(dependency):
				return os.path.realpath(dependency)

			return None
		except:
			print("source_info.get_real_path - Unknown error!")
			return None

	
	def __load_from_cache(self) -> None:
		if self.__cache_path is None:
			return
		try:
			with open(self.__cache_path, 'r') as file:
				text = file.read()
			self.__data = json.loads(text)
		except:
			self.__data = {}


	def __file_is_dirty(self, file_path : str) -> bool:
		if os.path.isfile(file_path):
			abs_path = file_path
		else:
			abs_path = self.get_real_path(file_path, None)
		if abs_path is None:
			print("source_info.__file_is_dirty - '" + file_path + "' not found!")
			return True

		if abs_path not in self.__data:
			return True
		file_data = self.__data[abs_path]
		if (source_info.date_key not in file_data) or (source_info.dependencies_key not in file_data):
			return True
		try:
			recorded_date = file_data[source_info.date_key]
			actual_date = os.path.getmtime(abs_path).__str__()
			return recorded_date != actual_date
		except:
			print("source_info.__file_is_dirty - Failed to get last modified date for '" + file_path + "'!")
			return True


	def __collect_file_dependencies(self, abs_path: str, dependencies : set):
		candidates = None
		# Get cached dependencies:
		if not self.__file_is_dirty(abs_path):
			candidates = []
			records = self.__data[abs_path][source_info.dependencies_key]
			for dependency in records:
				if not isinstance(dependency, str):
					continue
				dep_path = self.get_real_path(dependency, abs_path)
				if dep_path is None:
					candidates = None
					break
				elif dep_path not in dependencies:
					candidates.append(dep_path)
		
		# Read through the file and extract #include statements:
		if candidates is None:
			candidates = []
			self.__dirty_records.add(abs_path)
			try:
				with open(abs_path, 'r') as file:
					source = file.read()
			except:
				print("source_info.__collect_file_dependencies - Failed to open file '" + abs_path + "'!")
				return
			words = source.split()
			word_count = len(words)
			include_statement_len = len(source_info.include_statement)
			records = []
			for i in range(word_count):
				word = words[i]
				word_len = len(word)
				if word_len < include_statement_len:
					continue
				elif word_len == include_statement_len:
					if i >= (word_count - 1) or word != source_info.include_statement:
						continue
					decorated_path = words[i + 1]
				else:
					if word[0:include_statement_len] != source_info.include_statement:
						continue
					decorated_path = word[include_statement_len:]
				if len(decorated_path) < 2:
					continue
				if ((not (decorated_path[0] == decorated_path[-1] and decorated_path[0] == '"')) and
					(not (decorated_path[0] == '<' and decorated_path[-1] == '>'))):
					print("source_info.__collect_file_dependencies - Warning: Invalid #include statement detected in '" + abs_path + "': '" + decorated_path + "'!")
					continue
				dependency = decorated_path[1:-1]
				dep_path = self.get_real_path(dependency, abs_path)
				if dep_path is not None:
					candidates.append(dep_path)
					records.append(dependency)
			try:
				self.__data[abs_path] = { 
					source_info.date_key: os.path.getmtime(abs_path).__str__(),
					source_info.dependencies_key: records
				}
			except:
				print("source_info.__collect_file_dependencies - Warning: Failed to cache records for '" + abs_path + "'!")
				pass
		
		# Record found dependencies and recurse:
		for dep_path in candidates:
			if dep_path not in dependencies:
				dependencies.add(dep_path)
				self.__collect_file_dependencies(dep_path, dependencies)
