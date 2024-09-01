import sys, os
if __name__ == "__main__":
	sys.path.insert(0, os.path.realpath(os.path.dirname(os.path.realpath(__file__)) + "/../"))
from code_analysis.source_cache import source_cache
from code_analysis import jimara_tokenize_source


def evaluate_statement(statement: str) -> int:
	equasion_tokens = jimara_tokenize_source.tokenize_c_like(statement)
	python_script = ""
	i = 0
	while i < len(equasion_tokens):
		tok = equasion_tokens[i]
		i += 1
		if len(tok) <= 0:
			continue
		elif tok == 'false' or tok == 'False' or tok == 'True' or tok[0].isalpha():
			tok = ' 0 '
		elif tok == 'true':
			tok = ' 1 '
		elif tok == '||':
			tok = ' or '
		elif tok == '&&':
			tok = ' and '
		elif tok not in jimara_tokenize_source.single_symbol_tokens:
			tok = ' ' + tok + ' '
		python_script += tok
	return int(eval(python_script))


def source_string_repr(path) -> str:
	rv = '"'
	for s in path:
		if s == '\\':
			rv += '\\\\'
		elif s == '"':
			rv += '\\"'
		elif s == '\n':
			rv += '\\n'
		elif s == '\t':
			rv += '\\t'
		elif s == '\0':
			rv += '\\0'
		else:
			rv += s
	rv += '"'
	return rv


def generate_include_statement(path) -> str:
	return '#include ' + source_string_repr(path)


class source_line:
	def __init__(self, original_text: str, processed_text: str, file: str, line: int) -> None:
		self.original_text = original_text
		self.processed_text = processed_text
		self.file = file
		self.line = line

	def __str__(self) -> str:
		return '"' + self.file + '" ' + self.line.__str__() + ": " + self.processed_text

class macro_definition:
	def __init__(self, name: str, arg_names: dict, body_tokens: list) -> None:
		self.name = name
		self.arg_names = arg_names
		self.body_tokens = body_tokens

	def resolve(self, arg_list: list) -> str:
		result = ""
		for token in self.body_tokens:
			needs_spacing = (
				len(result) > 0 and (len(token) > 0) and
				(token not in jimara_tokenize_source.single_symbol_tokens))
			if needs_spacing:
				result += ' '

			if token in self.arg_names:
				index = self.arg_names[token]
				if index >= len(arg_list):
					# print("Could not resolve macro '" + self.name + "'! Too few arguments!")
					# return None
					continue
				else:
					result += arg_list[index]
			else:
				result += token

			if needs_spacing:
				result += ' '

		return result

	@staticmethod
	def parse(code: str):
		i = 0
		
		# Ignore leading whitespaces:
		while i < len(code) and code[i].isspace():
			i += 1

		# Read name:
		name = ''
		while i < len(code) and (code[i].isalnum() or code[i] == '_'):
			name += code[i]
			i += 1
		if len(name) <= 0:
			return "Macro does not have a name!"
		elif name[0].isnumeric():
			return "Macro name can not start with a digit ('" + name + "')!"
		
		# If we have a parameter list, extract parameter names:
		arg_names = {}
		if i < len(code) and code[i] == '(':
			word = ""
			arg_count = 0
			expecting_comma = False
			while i < len(code) and code[i] != ')':
				if code[i] == ',':
					if len(word) <= 0:
						return "Expected an identifier, found ','!"
					elif word[0].isnumeric():
						return "Argument identifier can not start with a digit ('" + word + "')!"
					elif word in arg_names:
						return "Duplicate macro argument name \'" + word + "\'!"
					else:
						arg_names[word] = arg_count
						arg_count += 1
						word = ""
						expecting_comma = False

				elif code[i].isspace():
					if len(word) > 0:
						expecting_comma = True
				
				elif code[i].isalnum() or code[i] == '_':
					if expecting_comma:
						return "Expecting ',' or ')', encountered '" + code[i] + "'!"
					else:
						word += code[i]
				i += 1

			# We HAVE TO see the closing ')' symbol!
			if i >= len(code):
				return "Closing ')' symbol missing at the end of parameter list!"
			i += 1
			
			# Append last word or fail if there's a comma before the ')' without word following it:
			if len(word) > 0:
				if word[0].isnumeric():
					return "Argument identifier can not start with a digit ('" + word + "')!"
				if word in arg_names:
					return "Duplicate macro argument name \'" + word + "\'!"
				else:
					arg_names[word] = arg_count
			elif len(arg_names) > 0:
				return "Expected argument name after ','!"

		# Rest is body; we just need to tokenize it and find argument name references:
		body = code[i:]
		body_tokens = jimara_tokenize_source.tokenize_c_like(body)
		if body_tokens is None:
			return "Could not tokenize body: '" + body + "'"

		# Done:
		result = macro_definition(name, arg_names, body_tokens)
		return result
	
	def __str__(self) -> str:
		return '[Name: "' + self.name + '"; Args: ' + repr(self.arg_names) + "; Body: " + repr(self.body_tokens) + ']'
	

class preporocessor_state:
	def __init__(self, src_cache: source_cache) -> None:
		self.src_cache = src_cache
		self.macro_definitions = {}
		self.pragma_handlers = {}
		self.custom_command_handlers = {}
		self.line_list: list[source_line]
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
			i += 1
		return result

	@staticmethod
	def __get_preprocessor_command(line) -> tuple[str, str]:
		i = 0
		while i < len(line) and line[i].isspace():
			i += 1
		if i >= len(line) or line[i] != '#':
			return None, None
		i += 1
		command = preporocessor_state.__read_keyword_or_name(line, i)
		return command, line[i+len(command):]
	
	@staticmethod
	def __clean_include_filename(src_file: str) -> str:
		sym_id = 0
		while sym_id < len(src_file) and src_file[sym_id].isspace():
			sym_id += 1
		if src_file[sym_id] == '"':
			file_end = '"'
		elif src_file[sym_id] == '<':
			file_end = '>'
		else:
			file_end = '\0'
		source_path = ""
		sym_id += 1
		while sym_id < len(src_file) and src_file[sym_id] != file_end:
			source_path += src_file[sym_id]
			sym_id += 1
		return source_path

	def resolve_macros(self, code_chunk: list) -> list:
		if code_chunk is None or len(code_chunk) <= 0:
			return None
		
		result = []
		failed = []
		def add_to_result(token: str, line_id: int):
			if result is None:
				return
			while len(result) <= line_id:
				orig = code_chunk[len(result)]
				result.append(source_line(orig.original_text, "", orig.file, orig.line))
			needs_spacing = (
				len(result[line_id].processed_text) > 0 and (len(token) > 0) 
				# and (token not in jimara_tokenize_source.single_symbol_tokens)
				)
			if needs_spacing:
				result[line_id].processed_text += ' '
			result[line_id].processed_text += token
			#if needs_spacing:
			#	result[line_id].processed_text += ' '
		
		def append_token_list(token_list):
			i = 0
			while i < len(token_list):
				token = token_list[i][0]
				line_id = token_list[i][1]
				orig_code_chunk = code_chunk[line_id]
				i += 1

				# Parse and resolve macro usage:
				if token in self.macro_definitions:
					macro_arg_list = []

					# Extract macro arguments:
					if (i < len(token_list)) and (token_list[i][0] == '('):
						i += 1
						nested_group_cnt = 0
						nested_scope_cnt = 0
						last_macro_arg = ""
						while (i < len(token_list)) and (len(failed) <= 0):
							body_token = token_list[i][0]
							i += 1
							if (body_token == ',') and (nested_group_cnt <= 0) and (nested_scope_cnt <= 0):
								macro_arg_list.append(last_macro_arg)
								last_macro_arg = ""
								continue
							elif body_token == '(':
								nested_group_cnt += 1
							elif body_token == '{':
								nested_scope_cnt += 1
							elif body_token == '}':
								nested_scope_cnt -= 1
								if (nested_scope_cnt < 0):
									print("Could not resolve macro '" + token + "'!" +
										" [File: '" + orig_code_chunk.file + "'; Line: " + orig_code_chunk.file.line.__str__() + "]")
									failed.append(True)
									break
							elif body_token == ')':
								if nested_group_cnt > 0:
									nested_group_cnt -= 1
								elif nested_scope_cnt > 0:
									print("Could not resolve macro '" + token + "'!" +
										" [File: '" + orig_code_chunk.file + "'; Line: " + orig_code_chunk.file.line.__str__() + "]")
									failed.append(True)
									break
								else:
									break
							last_macro_arg += body_token
						if (nested_group_cnt > 0) or (nested_scope_cnt > 0):
							print("Could not resolve macro '" + token + "'!" +
								" [File: '" + orig_code_chunk.file + "'; Line: " + orig_code_chunk.file.line.__str__() + "]")
							failed.append(True)
							break
						elif len(last_macro_arg) > 0:
							macro_arg_list.append(last_macro_arg)
					
					# Pass arguments to the macro and resolve recursively:
					resolved_macro = self.macro_definitions[token].resolve(macro_arg_list)
					resolved_tokens = [[macro_token, line_id] for macro_token in jimara_tokenize_source.tokenize_c_like(resolved_macro)]
					append_token_list(resolved_tokens)

				# Special case for defined(SOME_MACRO):
				elif token == "defined":
					if (((i + 2) >= len(token_list)) or 
						(token_list[i][0] != '(') or 
						(token_list[i + 2][0] != ')') or
						(not (len(token_list[i + 1][0]) > 0 and token_list[i + 1][0][0].isalpha()))):
						print("Could not resolve 'defined'!" +
							" [File: '" + orig_code_chunk.file + "'; Line: " + orig_code_chunk.file.line.__str__() + "]")
						failed.append(True)
						break
					else:
						add_to_result('1' if token_list[i + 1][0] in self.macro_definitions else '0', line_id)
						i += 3 # We skip-over "(", "MACRO_NAME" and ")" tokens..

				# Special case for "__FILE__" keyword:
				elif token == '__FILE__':
					token = '"'
					for s in orig_code_chunk.file:
						token += '\\\\' if s == '\\' else s
					token += '"'
					add_to_result(token, line_id)

				# Special case for "__LINE__" keyword:
				elif token == '__LINE__':
					add_to_result((orig_code_chunk.line + 1).__str__(), line_id)

				# By default, we keep the token 'as-is':
				else:
					add_to_result(token, line_id)

		all_tokens = []
		for i in range(len(code_chunk)):
			tokens = jimara_tokenize_source.tokenize_c_like(code_chunk[i].original_text)
			for token in tokens:
				all_tokens.append([token, i])
		append_token_list(all_tokens)
		return result if len(failed) <= 0 else None
	
	def evaluate_boolean_statement(self, command_body: source_line) -> bool:
		equasion = ""
		for line in self.resolve_macros([command_body]):
			equasion += line.processed_text + ' '
		try:
			rv = evaluate_statement(equasion) > 0
			# print ("Evaluated: '" + python_script + "' = " + rv.__str__())
			return rv
		except:
			print ("Could not evaluate: " + equasion)
			return None

	def include_file(self, src_file: str, including_file: str = None) -> bool:
		# Load file source:
		src_data = self.src_cache.get_content(src_file, including_file)
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
					expression_passed = False if self.line_disabled() else self.evaluate_boolean_statement(source_line(command_body, command_body, source_path, start_line_id))
					if expression_passed is None:
						print("#if preprocessor statement could not be evaluated!" +
							" [File: '" + source_path + "'; Line: " + start_line_id.__str__() + "]")
						expression_passed = False
					self.__if_results.append(expression_passed)
					self.__if_flags.append(if_flag_has_been_true if self.__if_results[-1] else if_flag_none)
				
				# '#ifdef SOME_MACRO' statement:
				elif command == "ifdef":
					self.__if_results.append(
						False if self.line_disabled() else
						(preporocessor_state.__read_keyword_or_name(command_body, 0) in self.macro_definitions))
					self.__if_flags.append(if_flag_has_been_true if self.__if_results[-1] else if_flag_none)

				# '#ifndef SOME_MACRO' statement:
				elif command == "ifndef":
					self.__if_results.append(
						False if self.line_disabled() else
						(preporocessor_state.__read_keyword_or_name(command_body, 0) not in self.macro_definitions))
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

				# '#else' statement after '#if/#ifdef/#elif':
				elif command == "else":
					if len(self.__if_results) <= 0:
						print("#else preprocessor statement encountered without corresponding #if, #ifdef or #ifndef!" +
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

				# If line is disabled, any other command will be ignored either case, so we go on:
				elif self.line_disabled():
					pass

				# If we have a cusrom command, send it to the handler:
				elif command in self.custom_command_handlers:
					self.custom_command_handlers[command](self.resolve_macros(
						[source_line(command_body, command_body, source_path, start_line_id)])[0])
						
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
						expression_passed = self.evaluate_boolean_statement(source_line(command_body, command_body, source_path, start_line_id))
						if expression_passed is not None:
							self.__if_results[-1] = expression_passed
							if self.__if_results[-1]:
								self.__if_flags[-1] |= if_flag_has_been_true
						else:
							print("#elif preprocessor statement could not be evaluated!" +
								" [File: '" + source_path + "'; Line: " + start_line_id.__str__() + "]")
							return False
					else:
						self.__if_results[-1] = False

				# Define macro:
				elif command == "define":
					macro_def = macro_definition.parse(command_body)
					if isinstance(macro_def, macro_definition):
						# print("Defined: " + macro_def.__str__())
						self.macro_definitions[macro_def.name] = macro_def
					else:
						print("Error parsing macro: " + macro_def.__str__() + " ('#define ", command_body, "') " +
							" [File: '" + source_path + "'; Line: " + start_line_id.__str__() + "]")
						return False

				# Remove macro definition:
				elif command == "undef":
					undefined_macro_name = preporocessor_state.__read_keyword_or_name(command_body, 0)
					if undefined_macro_name in self.macro_definitions:
						# print("Undefined: " + undefined_macro_name)
						del self.macro_definitions.remove[undefined_macro_name]

				# Include file:
				elif command == "include":
					file_root = self.resolve_macros([source_line(command_body, command_body, source_path, start_line_id)])
					if file_root == None or len(file_root) <= 0:
						return False
					elif not self.include_file(preporocessor_state.__clean_include_filename(file_root[0].original_text), src_file):
						return False
					
				# After commands may come pragmas:
				elif command == "pragma":
					pragma_command = preporocessor_state.__read_keyword_or_name(command_body, 0)

					# Handle custom pragmas:
					if pragma_command in self.pragma_handlers:
						pragma_body = command_body[len(pragma_command) + 1:]
						self.pragma_handlers[pragma_command](self.resolve_macros(
							[source_line(pragma_body, pragma_body, source_path, start_line_id)])[0])

					# Internally supported pragma is 
					elif pragma_command == "once":
						self.__once_files[source_path] = True
				
			# We just have a normal code line:	
			else:
				if not self.line_disabled():
					code_chunk.append(source_line(line, line, source_path, line_id))
				line_id += 1

		# Resolve macros in the last chunk and report success:
		code_chunk = self.resolve_macros(code_chunk)
		if code_chunk is not None:
			self.line_list.extend(code_chunk)
		return True


if __name__ == "__main__":
	cache = source_cache(["__Test__/include_dir_0", "__Test__/include_dir_1"])
	preprocessor = preporocessor_state(cache)
	preprocessor.include_file("subdir_a/file_a.h")
	for line in preprocessor.line_list:
		print(line)
