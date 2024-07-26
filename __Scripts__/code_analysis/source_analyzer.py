from preprocessor import source_line
from jimara_tokenize_source import tokenize_c_like

class __configuration:
	def __init__(self) -> None:
		self.pointer_size = 8

__config = __configuration()

virtual_qualifier = 'virtual'

class type_def:
	UNKNOWN = 0
	STRUCT = 1
	CLASS = 2
	UNION = 3
	ENUM = 4

	def __init__(self, 
			  category: int, names: list[str], parent_types: list['type_with_qualifiers'], 
			  variables: list['variable_def'], static_variables: list['variable_def'],
			  functions: list['function_def'], static_functions: list['function_def'],
			  base_alignmment: int, owner_namespace: 'namespace', qualifiers: list[str]) -> None:
		
		# Basic data
		self.__category = category
		self.__names = names
		self.__parent_types = parent_types
		self.__namespace = owner_namespace
		self.__qualifiers = qualifiers

		# We only allow 'power-of-two' alignment values here:
		self.__alignment = 1
		while self.__alignment < base_alignmment:
			self.__alignment += self.__alignment
		assert(self.__alignment == max(base_alignmment, 1))

		# Virtual size is be the total size of virtual parents and one vptr (plus padding for the next variable)
		# This thing is likely compiler-dependent and should not be trusted much, 
		# but we'll mostly be using size and offset values for GLSL analysis, so that's not a big deal...
		self.__virtual_data_size = 0
		for fun in functions:
			if virtual_qualifier in fun.qualifiers:
				self.__virtual_data_size = __config.pointer_size
				break
		def append_to_virtual_parents(parents: list['type_with_qualifiers']):
			for parent in parents:
				if virtual_qualifier not in parent.qualifiers:
					continue
				if self.__virtual_data_size <= 0:
					self.__virtual_data_size = __config.pointer_size
				append_to_virtual_parents(parent.type.parent_types())
				parent_alignment = parent.type.alignment()
				self.__alignment = max(self.__alignment, parent_alignment)
				self.__virtual_data_size = int((self.__virtual_data_size + parent_alignment - 1) / parent_alignment) * parent_alignment
				self.__virtual_data_size += parent.type.__non_virtual_parent_size + parent.type.__local_size
		append_to_virtual_parents(self.__parent_types)

		# Combined size of all non-virtual parents (plus padding for the first variable):
		self.__non_virtual_parent_size = 0
		for parent in self.parent_types():
			if virtual_qualifier in parent.qualifiers:
				continue
			parent_alignment = parent.type.alignment()
			self.__alignment = max(self.__alignment, parent_alignment)
			if self.__non_virtual_parent_size <= 0:
				self.__virtual_data_size = int((self.__virtual_data_size + parent_alignment - 1) / parent_alignment) * parent_alignment
			else:
				self.__non_virtual_parent_size = int((self.__non_virtual_parent_size + parent_alignment - 1) / parent_alignment) * parent_alignment
			self.__non_virtual_parent_size += parent.type.__non_virtual_parent_size + parent.type.__local_size
		
		# Local size will be the total size of contained variables
		self.__local_size = 0
		self.__variables: list['class_member_variable']
		self.__variables = []
		for variable in variables:
			var_alignment = variable.alignment()
			self.__alignment = max(self.__alignment, var_alignment)
			if self.__local_size <= 0:
				self.__non_virtual_parent_size = int((self.inhereted_size() + var_alignment - 1) / var_alignment) * var_alignment - self.__virtual_data_size
			else:
				self.__local_size = int((self.__local_size + var_alignment - 1) / var_alignment) * var_alignment
			self.__variables.append(class_member_variable(
				variable.type, variable.name, variable.array_size, variable.qualifiers, self.inhereted_size() + self.__local_size))
			self.__local_size += variable.packed_size()

		# Static variables
		self.__static_variables = static_variables
		
		# Functions
		self.__functions = functions
		self.__static_functions = static_functions

		# Qualifiers

	def names(self) -> list[str]:
		return self.__names
	
	def name(self) -> str:
		return None if (len(self.__names) <= 0) else self.__names[0]

	def category(self) -> int:
		return self.__category
	
	def owner_namespace(self) -> 'namespace':
		return self.__namespace

	def qualifiers(self) -> list[str]:
		return self.__qualifiers

	def alignment(self) -> int:
		return self.__alignment
	
	def virtual_data_size(self) -> int:
		return self.__virtual_data_size

	def non_virtual_parent_size(self) -> int:
		return self.__non_virtual_parent_size

	def local_variable_size(self) -> int:
		return self.__local_size
	
	def non_virtual_data_size(self) -> int:
		return self.__non_virtual_parent_size + self.__local_size

	def inhereted_size(self) -> int:
		return self.__virtual_data_size + self.__non_virtual_parent_size

	def packed_size(self) -> int:
		return self.__virtual_data_size + self.__non_virtual_parent_size + self.__local_size
	
	def size(self) -> int:
		return int((self.packed_size() + self.__alignment - 1) / self.__alignment) * self.__alignment
	
	def parent_types(self) -> list['type_with_qualifiers']:
		return self.__parent_types
	
	def local_variables(self) -> list['class_member_variable']:
		return self.__variables
	
	def static_variables(self) -> list['variable_def']:
		return self.__static_variables
	
	def functions(self) -> list['function_def']:
		return self.__functions
	
	def static_functions(self) -> list['function_def']:
		return self.__static_functions


class type_with_qualifiers:
	def __init__(self, type: type_def, qualifiers: list[str]) -> None:
		self.type = type
		self.qualifiers = qualifiers


class variable_def:
	def __init__(self, type: type_def, name: str, array_size: int, qualifiers: list[str]) -> None:
		self.type = type
		self.name = name
		self.array_size = max(array_size, 0)
		self.qualifiers = qualifiers

	def alignment(self) -> int:
		return self.type.alignment()
	
	def packed_size(self) -> int:
		if self.array_size <= 0:
			return 0
		else:
			return self.type.size() * (self.array_size - 1) + self.type.packed_size()
		
	def size(self) -> int:
		return self.type.size() * self.array_size


class class_member_variable(variable_def):
	def __init__(self, type: type_def, name: str, array_size: int, qualifiers: list[str], offset: int) -> None:
		variable_def.__init__(type, name, array_size, qualifiers)
		self.offset = offset


class function_def:
	def __init__(self, return_type: type_def, variables: list[variable_def], body: str, qualifiers: list[str]) -> None:
		self.return_type = return_type
		self.variables = variables
		self.body = body
		self.qualifiers = qualifiers


class namespace:
	def __init__(self, 
			  name: str, 
			  types: dict[str, type_def],
			  variables: dict[str, variable_def],
			  functions: dict[str, function_def],
			  sub_namespaces: dict[str,]) -> None:
		self.name = name
		self.types = types
		self.variables = variables
		self.functions = functions
		self.sub_namespaces = sub_namespaces


def extract_types(lines: list[source_line], scope_namespace: namespace):
	# Break stuff into words:
	words = []
	for src_line in lines:
		line_words = tokenize_c_like(src_line.processed_text)
		for line_word in line_words:
			words.append((line_word, src_line))
	
	# __TODO__: Find statements and/or scopes:
	pass
