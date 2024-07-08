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
		self.macro_definitions = []
		self.line_list = []
		self.__if_state = []
		self.__if_flags = []
		self.__once_files = {}

	def line_disabled(self) -> bool:
		return len(self.__if_state) > 0 and (not self.__if_state[-1])
	
	@staticmethod
	def __read_keyword_or_name(text: str, startIndex: int) -> str:
		i = startIndex
		while i < len(text) and text[i].isspace():
			i += 1
		result = ""
		while i < len(text) and (text[i].isalnum() or text[i] == '_'):
			result += text[i]
		return result

	@staticmethod
	def __get_preprocessor_command(line) -> tuple:
		i = 0
		while i < len(line) and line[i].isspace():
			i += 1
		if i >= len(line) or line[i] != '#':
			return None, None
		command = preporocessor_state.__read_keyword_or_name(line, i)
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
	
	def evaluate_boolean_statement(self, command_body: str) -> bool:
		equasion = self.resolve_macros(command_body)
		# __TODO__: Actually evaluate the equasion!
		return False

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

		# A few constants for the #if/#ifdef/#elif/#else/#endif statements:
		if_flag_none = 0
		if_flag_else_encountered = 1
		if_flag_has_been_true = 2

		line_id = 0
		code_chunk = []
		while line_id < len(code_lines):
			line: str = code_lines[line_id]
			command, command_body = preporocessor_state.__get_preprocessor_command(line)
			if command is not None:
				# Macro resolution can only happen in-between preprocessor commands, so if any lines are accumulated so far, here's the time for resolution:
				code_chunk = self.resolve_macros(code_chunk)
				if code_chunk is not None:
					self.line_list.extend(code_chunk)
				code_chunk = []

				# If we had preprocesso line-escape symbol, we should add to the code chunk:
				start_line_id = line_id
				while len(command_body) > 0 and command_body[-1] == '\\':
					line_id += 1
					if line_id < len(code_lines):
						command_body = command_body[0:-1] + code_lines[line_id]
				line_id += 1

				# We have a preprocessor line:
				if command == "if":
					self.__if_flags.append(self.evaluate_boolean_statement(command_body))
					self.__if_flags.append(if_flag_has_been_true if self.__if_results[-1] else if_flag_none)
				
				elif command == "ifdef":
					self.__if_results.append(preporocessor_state.__read_keyword_or_name(command_body, 0) in self.macro_definitions)
					self.__if_flags.append(if_flag_has_been_true if self.__if_results[-1] else if_flag_none)
				
				elif command == "else":
					if len(self.__if_results) <= 0:
						print(
							"#else preprocessor statement encountered without corresponding #if or #ifdef!" +
							" [File: '" + source_path + "'; Line: " + start_line_id.__str__() + "]")
						return False
					elif (self.__if_flags[-1] & if_flag_else_encountered) != 0:
						print(
							"Encountered two #else preprocessor statements in a row!" +
							" [File: '" + source_path + "'; Line: " + start_line_id.__str__() + "]")
						return False
					else:
						self.__if_results[-1] = not self.__if_results[-1]
						self.__if_flags[-1] |= if_flag_else_encountered
						if self.__if_results[-1]:
							self.__if_flags[-1] |= if_flag_has_been_true
						
				elif command == "elif":
					if len(self.__if_results) <= 0:
						print(
							"#elif preprocessor statement encountered without corresponding #if or #ifdef!" +
							" [File: '" + source_path + "'; Line: " + start_line_id.__str__() + "]")
						return False
					elif (self.__if_flags[-1] & if_flag_else_encountered) != 0:
						print(
							"#elif preprocessor statement encountered after #else!" +
							" [File: '" + source_path + "'; Line: " + start_line_id.__str__() + "]")
						return False
					elif (self.__if_flags[-1] & if_flag_has_been_true) == 0:
						self.__if_results[-1] = self.evaluate_boolean_statement(command_body)
						if self.__if_results[-1]:
							self.__if_flags[-1] |= if_flag_has_been_true
					else:
						self.__if_results[-1] = False
						
				elif command == "endif":
					if len(self.__if_results) <= 0:
						print(
							"#endif preprocessor statement encountered without corresponding #if or #ifdef!" +
							" [File: '" + source_path + "'; Line: " + start_line_id.__str__() + "]")
						return False
					else:
						self.__if_results.pop()
						self.__if_flags.pop()
					
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
						file_root = self.resolve_macros(self, [source_line(command_body, command_body, source_path, start_line_id)])
						if file_root == None or len(file_root) <= 0:
							return False
						elif not self.include_file(preporocessor_state.__clean_include_filename(file_root[0].processed_text)):
							return False
			else:
				code_chunk.append(source_line(line, line, source_path, line_id))
				line_id += 1

		# Resolve macros in the last chunk and report success:
		code_chunk = self.resolve_macros(code_chunk)
		if code_chunk is not None:
			self.line_list.extend(code_chunk)
		return True
