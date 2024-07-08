import source_cache

class source_line:
	def __init__(self, original_text: str, processed_text: str, file: str, line: int) -> None:
		self.original_text = original_text
		self.processed_text = processed_text
		self.file = file
		self.line = line

	def __str__(self) -> str:
		return '"' + self.file + '" ' + self.line.__str__() + ": " + self.content

class macro_definition:
	def __init__(self, name: str, arg_names: list, body: str) -> None:
		self.name = name
		self.arg_names = arg_names
		self.body = body

	@staticmethod
	def parse(code: str):
		return None

class preporocessor_state:
	def __init__(self, src_cache: source_cache.source_cache) -> None:
		self.src_cache = src_cache
		self.line_list = []
		self.__if_results = []
		self.__once_files = {}

	def line_disabled(self) -> bool:
		return len(self.__if_results) > 0 and (not self.__if_results[len(self.__if_results) - 1])
	
	@staticmethod
	def __get_preprocessor_command(line) -> tuple:
		i = 0
		while i < len(line) and line[i].isspace():
			i += 1
		if i >= len(line) or line[i] != '#':
			return None, None
		command = ""
		i += 1
		while i < len(line) and line[i].isspace():
			i += 1
		while i < len(line) and (line[i].isalnum() or line[i] == '_'):
			command += line[i]
		return command, line[i+1:]
	
	@staticmethod
	def __clean_include_filename(src_file) -> str:
		if src_file[0] == '"':
			file_end = '"'
		elif src_file[0] == '<':
			file_end = '>'
		else:
			file_end = '\0'
		source_path = ""
		sym_id = 1
		while sym_id < len(src_file) and src_file[sym_id] != file_end:
			source_path += src_file[sym_id]
		return source_path
	
	def resolve_macros(self, code_chunk: list) -> list:
		if code_chunk is None or len(code_chunk) <= 0:
			return None
		def iterate() -> bool:
			# __TODO__: Actually resolve macros (1 level, without recursion)!
			return False
		while iterate():
			pass
		return code_chunk

	def include_file(self, src_file: str) -> bool:
		# Load file source:
		src_data = self.src_cache.get_content(src_file)
		if src_data == None:
			print("Could not read file: \"" + src_file + "\"!")
			return False
		
		# Skip if the file had #pragma once on it's top
		source_path = src_data.path
		if source_path in self.__once_files:
			return True
		code_lines = src_data.code_lines

		line_id = 0
		code_chunk = []
		while line_id < len(code_lines):
			line: str = line_id[line_id]
			command, command_body = preporocessor_state.__get_preprocessor_command(line)
			if command is not None:
				# Macro resolution can only happen in-between preprocessor commands, so if any lines are accumulated so far, here's the time for resolution:
				code_chunk = self.resolve_macros(code_chunk)
				if code_chunk is not None:
					self.line_list.extend(code_chunk)
				code_chunk = []

				# We have a preprocessor line:
				if command == "if":
					pass
				elif command == "elif":
					pass
				elif command == "endif":
					pass
				elif not self.line_disabled():
					if command == "pragma":
						# __TODO__: Support 'once' by default; add more too!
						pass
					elif command == "define":
						# __TODO__: Parse macro definition!
						pass
					if command == "undef":
						# __TODO__: Remove macro definition!
						pass
					elif command == "include":
						# Include file:
						file_root = self.resolve_macros(self, [source_line(command_body, command_body, source_path, line_id)])
						if file_root == None or len(file_root) <= 0:
							return False
						elif not self.include_file(preporocessor_state.clean_file_name(file_root[0].processed_text)):
							return False
			else:
				code_chunk.append(source_line(line, line, source_path, line_id))

			line_id += 1

		# Resolve macros in the last chunk and report success:
		code_chunk = self.resolve_macros(code_chunk)
		if code_chunk is not None:
			self.line_list.extend(code_chunk)
		return True
