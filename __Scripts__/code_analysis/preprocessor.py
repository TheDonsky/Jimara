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
		self.macro_definitions = {}
		self.pragma_handlers = {}
		self.custom_command_handlers = {}
		self.line_list = []
		self.__if_results = []
		self.__if_flags = []
		self.__once_files = {}

	def line_disabled(self) -> bool:
		return False in self.__if_results
	
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

		# Loop-over lines:
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
						command_body = command_body[0:-1] + '\n' + code_lines[line_id]
				line_id += 1

				# '#if (boolean expression)' statement:
				if command == "if":
					self.__if_flags.append(False if self.line_disabled() else self.evaluate_boolean_statement(command_body))
					self.__if_flags.append(if_flag_has_been_true if self.__if_results[-1] else if_flag_none)
				
				# '#ifdef SOME_MACRO' statement:
				elif command == "ifdef":
					self.__if_results.append(
						False if self.line_disabled() else
						(preporocessor_state.__read_keyword_or_name(command_body, 0) in self.macro_definitions))
					self.__if_flags.append(if_flag_has_been_true if self.__if_results[-1] else if_flag_none)

				# '#endif' statement:
				elif command == "endif":
					if len(self.__if_results) <= 0:
						print("#endif preprocessor statement encountered without corresponding #if or #ifdef!" +
							" [File: '" + source_path + "'; Line: " + start_line_id.__str__() + "]")
						return False
					else:
						self.__if_results.pop()
						self.__if_flags.pop()

				# If line is disabled, any other command will be ignored either case, so we go on:
				elif self.line_disabled():
					pass

				# If we have a cusrom command, send it to the handler:
				elif command in self.custom_command_handlers:
					self.custom_command_handlers[command](self.resolve_macros(
						[source_line(command_body, command_body, source_path, start_line_id)])[0])
				
				# '#else' statement after '#if/#ifdef/#elif':
				elif command == "else":
					if len(self.__if_results) <= 0:
						print("#else preprocessor statement encountered without corresponding #if or #ifdef!" +
							" [File: '" + source_path + "'; Line: " + start_line_id.__str__() + "]")
						return False
					elif (self.__if_flags[-1] & if_flag_else_encountered) != 0:
						print("Encountered two #else preprocessor statements in a row!" +
							" [File: '" + source_path + "'; Line: " + start_line_id.__str__() + "]")
						return False
					else:
						self.__if_results[-1] = not self.__if_results[-1]
						self.__if_flags[-1] |= if_flag_else_encountered
						if self.__if_results[-1]:
							self.__if_flags[-1] |= if_flag_has_been_true
						
				# '#elif (boolean expression)' after '#if/#ifdef/#elif':
				elif command == "elif":
					if len(self.__if_results) <= 0:
						print("#elif preprocessor statement encountered without corresponding #if or #ifdef!" +
							" [File: '" + source_path + "'; Line: " + start_line_id.__str__() + "]")
						return False
					elif (self.__if_flags[-1] & if_flag_else_encountered) != 0:
						print("#elif preprocessor statement encountered after #else!" +
							" [File: '" + source_path + "'; Line: " + start_line_id.__str__() + "]")
						return False
					elif (self.__if_flags[-1] & if_flag_has_been_true) == 0:
						self.__if_results[-1] = self.evaluate_boolean_statement(command_body)
						if self.__if_results[-1]:
							self.__if_flags[-1] |= if_flag_has_been_true
					else:
						self.__if_results[-1] = False

				# Define macro:
				elif command == "define":
					macro_def = macro_definition.parse(command_body)
					if macro_def is not None:
						self.macro_definitions[macro_def.name] = macro_def

				# Remove macro definition:
				if command == "undef":
					undefined_macro_name = preporocessor_state.__read_keyword_or_name(command_body, 0)
					if undefined_macro_name in self.macro_definitions:
						del self.macro_definitions.remove[undefined_macro_name]

				# Include file:
				elif command == "include":
					file_root = self.resolve_macros(self, [source_line(command_body, command_body, source_path, start_line_id)])
					if file_root == None or len(file_root) <= 0:
						return False
					elif not self.include_file(preporocessor_state.__clean_include_filename(file_root[0].processed_text)):
						return False
					
				# After commands may come pragmas:
				elif command == "pragma":
					pragma_command = preporocessor_state.__read_keyword_or_name(command_body, 0)

					# Handle custom pragmas:
					if pragma_command in self.pragma_handlers:
						pragma_body = command_body[len(pragma_command):]
						self.pragma_handlers[pragma_command](self.resolve_macros(
							[source_line(pragma_body, pragma_body, source_path, start_line_id)])[0])

					# Internally supported pragma is 
					elif pragma_command == "once":
						self.__once_files[source_path] = True
					
			elif not self.line_disabled():
				# We just have a normal code line:
				code_chunk.append(source_line(line, line, source_path, line_id))
				line_id += 1

		# Resolve macros in the last chunk and report success:
		code_chunk = self.resolve_macros(code_chunk)
		if code_chunk is not None:
			self.line_list.extend(code_chunk)
		return True
