from source_cache import source_cache
from preprocessor import source_line, preporocessor_state
import syntax_tree_extractor
from syntax_tree_extractor import syntax_tree_node
import jimara_tokenize_source

CPP_QUALIFIERS = {
	# 'Just words' as far as our use-case goes:
	'const',
	'consteval',
	'constexpr',
	'constinit',
	'explicit',
	'export',
	'extern',
	'friend',
	'inline',
	'mutable',
	'noexcept',
	'private',
	'protected',
	'public',
	'static',
	'register',
	'thread_local',
	'volatile',
	'virtual',
	'typename',
			
	# Qualifiers with parameters:
	'alignas',
	'__declspec'
}
	
CPP_KEYWORDS = {
	'class', 'struct', 'enum', 'union',
	'void', 'template', 'using',
	'typedef', 'typeof',
	'operator',
	'for', 'while', 'if', 'else', 'do', 'goto',
	'asm',
	'static_cast', 'dynamic_cast', 'reinterpret_cast'
}

class __configuration:
	def __init__(self) -> None:
		self.pointer_size = 8
		self.qualifiers = CPP_QUALIFIERS
		self.keywords_and_operands = (
			self.qualifiers
			.union(CPP_KEYWORDS)
			.union(jimara_tokenize_source.operand_tokens)
			.difference({'::'})
		)

__config = __configuration()

alignment_qualifier = 'alignas'

class type_def:
	''' 
	Definition of a type \n
	Built-in primitive types are an exception to the rule in a sense, because they only have no member variables
	'''

	UNKNOWN = 0 
	''' Undefined/Unknown type category '''

	STRUCT = 1
	'''
	struct-type category: \n
	Effectively the same as class; variables/functions are public by default
	'''

	CLASS = 2
	''' 
	class-type category: \n
	Effectively the same as struct; variables/functions are private by default
	'''

	UNION = 3
	''' 
	union type category: \n
	No inheritance allowed; members occupy the same space
	'''

	ENUM = 4
	''' 
	enum or enum class type category: \n
	Has no member variables and parent type can only be one of the built-in integer types; 
	values are defined as static const variables; enum class has 'full Type::Val' names
	'''

	def __init__(self, containing_namespace: 'namespace',
			  category: int, name: str, parent_typenames: list[str], qualifiers: list[syntax_tree_node],
			  member_variables: list['variable_def'], static_variables: list['variable_def'],
			  member_functions: list['function_def'], static_functions: list['function_def']) -> None:
		
		self.containing_namespace = containing_namespace
		self.namespace = namespace
		self.category = category
		self.name = name
		self.parent_typenames = parent_typenames
		self.qualifiers = qualifiers
		self.member_variables = member_variables
		self.static_variables = static_variables
		self.member_functions = member_functions
		self.static_functions = static_functions


class variable_def:
	def __init__(self, type: type_def, name: str, array_size: int, qualifiers: list[syntax_tree_node]) -> None:
		self.type = type
		self.name = name
		self.array_size = max(array_size, 0)
		self.qualifiers = qualifiers


class function_def:
	def __init__(self, return_type: type_def, arguments: list[variable_def], body: syntax_tree_node, qualifiers: list[syntax_tree_node]) -> None:
		self.return_type = return_type
		self.arguments = arguments
		self.body = body
		self.qualifiers = qualifiers


class namespace:
	def __init__(self, 
			  parent_namespace: 'namespace', 
			  name: str,
			  types: dict[str, type_def],
			  used_namespaces: set['namespace'],
			  variables: dict[str, variable_def],
			  functions: dict[str, list[function_def]],
			  sub_namespaces: dict[str, 'namespace']) -> None:
		self.parent_namespace = parent_namespace
		self.name = name
		self.types = types
		self.used_namespaces = used_namespaces
		self.variables = variables
		self.functions = functions
		self.sub_namespaces = sub_namespaces

	def __find_definition(self, name, find_fn):
		name_tokens = jimara_tokenize_source.tokenize_c_like(name)
		if (name_tokens is None) or (len(name_tokens) <= 0):
			return None
		
		is_not_absolute_path = (name_tokens[0] != '::')
		name_tokens = [elem for elem in name_tokens if elem != '::']
		if len(name_tokens) <= 0:
			return None
		
		def find_within_scope(scope: namespace) -> type_def:
			for i in range(len(name_tokens) - 1):
				if name_tokens[i] not in scope.sub_namespaces:
					return None
				# __TODO__: We might be dealling with nested classes! Support those too!
				scope = scope.sub_namespaces[name_tokens[i]]
			return find_fn(name_tokens[-1])
		
		space = self
		while space is not None:
			if is_not_absolute_path or (space.parent_namespace is None):
				res = find_within_scope(space)
				if res is not None:
					return res
				for used_namespace in space.used_namespaces:
					res = used_namespace.__find_definition(name, find_fn)
					if res is not None:
						return res
			space = space.parent_namespace
		return None

	def find_type_definition(self, typename: str) -> type_def:
		def find_type(scope: namespace, local_name: str) -> type_def:
			if local_name in scope.types:
				return scope.types[local_name]
			return None
		return self.__find_definition(typename, find_type)
	
	def find_variable_definition(self, name: str) -> type_def:
		def find_variable(scope: namespace, local_name: str) -> type_def:
			if local_name in scope.variables:
				return scope.variables[local_name]
			return None
		return self.__find_definition(name, find_variable)
	
	def find_function_definitions(self, name: str) -> type_def:
		def find_functions(scope: namespace, local_name: str) -> type_def:
			if local_name in scope.functions:
				return scope.functions[local_name]
			return None
		return self.__find_definition(name, find_functions)


def unify_namespace_paths(tree_nodes: list[syntax_tree_node], recurse: bool = True) -> list[syntax_tree_node]:
	result: list[syntax_tree_node]
	result = []
	for cur_node in tree_nodes:
		if ((not (cur_node.has_child_nodes())) and
			(len(result) > 0) and 
			(not (result[-1].has_child_nodes())) and 
			(cur_node.token.token not in __config.keywords_and_operands) and
			(result[-1].token.token not in __config.keywords_and_operands) and
			((cur_node.token.token == '::') or (result[-1].token.token.endswith('::')))):
			result[-1].token.token += cur_node.token.token
		elif recurse and cur_node.has_child_nodes():
			grouped_sub_nodes = unify_namespace_paths(cur_node.child_nodes, recurse)
			result.append(syntax_tree_node(cur_node.token, grouped_sub_nodes))
		else:
			result.append(cur_node)
	return result


def parse_namespace_content(nodes: list[syntax_tree_node], scope: namespace) -> None:
	node_id = 0
	
	qualifier_list: list[syntax_tree_node]
	qualifier_list = []

	while node_id < len(nodes):
		node = nodes[node_id]
		node_id += 1


		# Just a qualifier; we can append it to the list and move on:
		if node.token.token in __config.qualifiers:
			qualifier_list.append(node)
			continue


		# Random in-namespace scopes are generally not allowed, but we can have () parameters after qualifiers, so let's ignore those and report others.
		# If there's a syntax error, actual compiler will take care of the problem..
		elif node.has_child_nodes():
			if (node.token.token == '(') and (len(qualifier_list) > 0) and (qualifier_list[-1].token.token in __config.qualifiers):
				qualifier_list.append(node)
			else:
				print(
					"Expected a keyword or a qualifier! Encountered '" + node.token.token + 
		   			"' [File: " + node.token.line.file + "; Line: " + str(node.token.line.line) + "]")
				exit(1)
			continue


		# Add entries to a sub-namespace
		elif node.token.token == 'namespace':
			qualifier_list = []
			namespace_scope_node = None
			sub_namespace_path = ""
			while node_id < len(nodes):
				scope_node = nodes[node_id]
				node_id += 1
				if scope_node.token.token == ';':
					break
				elif scope_node.token.token == '{':
					namespace_scope_node = scope_node
					break
				elif (len(sub_namespace_path) <= 0) and (scope_node.token.token not in __config.keywords_and_operands):
					sub_namespace_path = scope_node.token.token
			if (namespace_scope_node is None) or (not namespace_scope_node.has_child_nodes()):
				continue

			# Do we NEED to divide sub-namespace name with '::'? Don't think that's universally supported
			sub_namespace_tokens = [elem for elem in jimara_tokenize_source.tokenize_c_like(sub_namespace_path) if elem != '::']
			sub_namespace_scope = scope
			for token in sub_namespace_tokens:
				if token in sub_namespace_scope.sub_namespaces:
					sub_namespace_scope = sub_namespace_scope.sub_namespaces[token]
				else:
					sub_namespace_scope = namespace(sub_namespace_scope, token, {}, {}, {}, {}, {})
					sub_namespace_scope.parent_namespace.sub_namespaces[token] = sub_namespace_scope
			parse_namespace_content(namespace_scope_node.child_nodes, sub_namespace_scope)
			continue


		# Inline assmebly can be ignored:
		elif node.token.token == 'asm':
			qualifier_list = []
			while node_id < len(nodes) and nodes[node_id].token.token != ';':
				node_id += 1
			node_id += 1
			continue


		# If we encounter a premature ';' symbol without actually figuring out what statement we are dealling with, we can just ignore it 
		# If this is an error, let the compiler deal with it. We can just move on.
		elif node.token.token == ';':
			qualifier_list = []
			continue

		
		elif node.token.token == 'template':
			# We are dealling with a template! Should we skip it? Should we parse it? Can be a function, can be something else entirely...
			continue

		elif node.token.token == 'struct':
			# __TODO__: Parse a struct, forward-declaration or a variable/function with 'struct' prefixed to it!
			continue

		elif node.token.token == 'class':
			# __TODO__: Parse a class, it's forward-declaration or a variable/function with 'class' prefixed to it!
			continue

		elif node.token.token == 'union':
			# __TODO__: Parse an union, it's forward-declaration or a variable/function with 'union' prefixed to it!
			continue

		elif node.token.token == 'enum':
			# __TODO__: Parse an enum/enum class, it's forward-declaration or a variable/function with 'enum' prefixed to it!
			continue

		elif node.token.token == 'typedef':
			# __TODO__: Parse a 'typedef' command; can be a class/struct definition or alias creation!
			continue

		elif node.token.token == 'using':
			# __TODO__: Parse a 'using' command; can be a class/struct definition, alias creation or namespace use!
			continue

		elif node.token.token == 'void':
			# __TODO__: Parse a void function!
			continue

		elif node.token.token == 'typeof':
			# __TODO__: Parse a variable or a function with type stated as 'typeof(SomeVar/Class)'!
			continue

		elif node.token.token.endswith("::"):
			# __TODO__: We may be dealling with a destructor or an operator...
			continue

		else:
			# __TODO__: Parse a variable or a function with type equal to node.token.token!
			# Note that we may be unable to include some files and it's not strictly necessary to have those definitions validated
			# We can also be dealling with a constructor definition here...
			continue

	pass


if __name__ == "__main__":
	cache = source_cache(["__Test__/include_dir_0", "__Test__/include_dir_1"])
	preprocessor = preporocessor_state(cache)
	preprocessor.include_file("subdir_a/file_a.h")
	tokens = syntax_tree_extractor.tokenize_source_lines(preprocessor.line_list)
	raw_syntax_tree = syntax_tree_extractor.extract_syntax_tree(tokens)[0]
	syntax_tree = unify_namespace_paths(raw_syntax_tree)
	for node in syntax_tree:
		print(node.to_str('  '))
