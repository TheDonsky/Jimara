from preprocessor import source_line
from jimara_tokenize_source import tokenize_c_like

class namespace:
	def __init__(self, name: str, types: dict, sub_namespaces: dict, functions: dict) -> None:
		self.name = name
		self.types = types
		self.sub_namespaces = sub_namespaces
		self.functions = functions

class type:
	UNKNOWN = 0
	STRUCT = 1
	CLASS = 2
	UNION = 3
	ENUM = 4

	def __init__(self, names: list, category: int, members: list, static_members: list, alignmment: int) -> None:
		self.names = names
		self.category = category
		self.members = members
		self.static_members = static_members
		self.alignment = alignmment

class variable:
	def __init__(self, type: type, name: str, offset: int) -> None:
		self.type = type
		self.name = name
		self.offset = offset

class enum_value:
	def __init__(self, name: str, value: int) -> None:
		self.name = name
		self.value = value

class function:
	def __init__(self, return_type: type, variables: list, body: str, qualifiers: str) -> None:
		self.return_type = return_type
		self.variables = variables
		self.body = body

def extract_types(lines: list, scope_namespace: namespace):
	# Break stuff into words:
	words = []
	src_line: source_line
	for src_line in lines:
		line_words = tokenize_c_like(src_line.processed_text)
		for line_word in line_words:
			words.append((line_word, src_line))
	
	# __TODO__: Find statements and/or scopes:
	pass

