import sys, os
if __name__ == "__main__":
	sys.path.insert(0, os.path.realpath(os.path.dirname(os.path.realpath(__file__)) + "/../"))
from code_analysis.source_cache import source_cache
from code_analysis.preprocessor import preporocessor_state, source_line, macro_definition, evaluate_statement
from code_analysis.syntax_tree_extractor import extract_syntax_tree_from_source_line
from code_analysis.jimara_tokenize_source import tokenize_c_like


JM_MaterialPath_pragma_name = 'JM_MaterialPath'
JM_BlendMode_pragma_name = 'JM_BlendMode'
JM_MaterialFlags_pragma_name = 'JM_MaterialFlags'
JM_MaterialPropety_pragma_name = 'JM_MaterialProperty'
JM_FragmentField_pragma_name = 'JM_FragmentField'
JM_ShadingStateSize_pragma_name = 'JM_ShadingStateSize'


class type_info:
	def __init__(self, glsl_name: str, cpp_name: str, size: int, alignment: int) -> None:
		self.glsl_name = glsl_name
		self.cpp_name = cpp_name
		self.size = size
		self.alignment = alignment

	def __str__(self) -> str:
		return (
			'\{glsl_name: ' + self.glsl_name + 
			'; cpp_name: ' + self.cpp_name + 
			'; size: ' + str(self.size) + 
			'; alignment: ' + str(self.alignment) + '\}')


built_in_primitive_typenames = dict[str, type_info]
built_in_primitive_typenames = {
	'float':	type_info('float', 'float', 4, 4),
	'double':	type_info('double', 'double', 8, 8),
	'int':		type_info('int', 'int', 4, 4),
	'int32_t':	type_info('int32_t', 'int32_t', 4, 4),
	'uint':		type_info('uint', 'uint32_t', 4, 4),
	'uint32_t':	type_info('uint32_t', 'uint32_t', 4, 4),
	'bool':		type_info('bool', 'uint32_t', 4, 4), # Glsl uses 32 bit booleans, no?
	'int64_t':	type_info('int64_t', 'int64_t', 8, 8),
	'uint64_t':	type_info('uint64_t', 'uint64_t', 8, 8),
	'vec2':		type_info('vec2', '::Jimara::Vector2', 8, 8),
	'vec3':		type_info('vec3', '::Jimara::Vector3', 12, 16),
	'vec4':		type_info('vec4', '::Jimara::Vector4', 16, 16),
	'mat4':		type_info('mat4', '::Jimara::Matrix4', 64, 16),
	'sampler2D': type_info('sampler2D', '::Jimara::Graphics::TextureSampler', 4, 4) # We will be using 4-byte index to the sampler...
}


JM_Blend_Opaque_name =		'JM_Blend_Opaque'
JM_Blend_Opaque_value =		0
JM_Blend_Alpha_name =		'JM_Blend_Alpha'
JM_Blend_Alpha_value =		1
JM_Blend_Additive_name =	'JM_Blend_Additive'
JM_Blend_Additive_value =	2
blend_mode_options: dict[str, int]
blend_mode_options = {
	JM_Blend_Opaque_name:	JM_Blend_Opaque_value,
	JM_Blend_Alpha_name:	JM_Blend_Alpha_value,
	JM_Blend_Additive_name:	JM_Blend_Additive_value
}


JM_CanDiscard_name =					'JM_CanDiscard'
JM_CanDiscard_value =					(1 << 0)
JM_UseObjectId_name =					'JM_UseObjectId'
JM_UseObjectId_value =					(1 << 1)
JM_UsePerVertexTilingAndOffset_name =	'JM_UsePerVertexTilingAndOffset'
JM_UsePerVertexTilingAndOffset_value =	(1 << 2)
JM_UseVertexColor_name =				'JM_UseVertexColor'
JM_UseVertexColor_value =				(1 << 3)
material_flags: dict[str, int]
material_flags = {
	JM_CanDiscard_name:						JM_CanDiscard_value,
	JM_UseObjectId_name:					JM_UseObjectId_value,
	JM_UsePerVertexTilingAndOffset_name:	JM_UsePerVertexTilingAndOffset_value,
	JM_UseVertexColor_name:					JM_UseVertexColor_value
}


class material_path:
	def __init__(self, name: str, path: str, hint: str) -> None:
		self.name = name
		self.path = path
		self.hint = hint

	def __str__(self) -> str:
		return '(name: ' + self.name + '; path: ' + self.path + '; hint: ' + self.hint + ')'


class material_property:
	def __init__(self, value_type: type_info, variable_name: str, default_value: str, editor_alias: str, hint: str) -> None:
		self.value_type = value_type
		self.variable_name = variable_name
		self.default_value = default_value
		self.editor_alias = self.self.variable_name if (editor_alias is None) else editor_alias
		self.hint = (self.self.variable_name + ' [' + self.typename + ']') if (hint is None) else hint

	def __str__(self) -> str:
		return (
			'(type: ' + self.value_type.cpp_name + 
			'; name: ' + self.variable_name + 
			'; default: ' + self.default_value + 
			'; alias: ' + self.editor_alias + 
			'; hint: ' + self.hint + ')')


class fragment_field:
	def __init__(self, typename: str, variable_name: str) -> None:
		self.typename = typename
		self.variable_name = variable_name

	def __str__(self) -> str:
		return '(type: ' + self.typename + '; name: ' + self.variable_name + ')'


class lit_shader_data:
	def __init__(self, path: material_path) -> None:
		self.path = path
		self.editor_paths: list[material_path]
		self.editor_paths = []
		self.blend_mode = JM_Blend_Opaque_name
		self.material_flags = 0
		self.material_properties: list[material_property]
		self.material_properties = []
		self.fragment_fields: list[fragment_field]
		self.fragment_fields = []
		self.shading_state_size = 0

	def __str__(self) -> str:
		res = '{\n'
		res += '    Shader path: ' + str(self.path) + '\n'
		res += '    Editor paths: {\n'
		for path in self.editor_paths:
			res += '        ' + str(path) + '\n'
		res += '    }\n'
		res += '    Blend Mode: ' + str(self.blend_mode) + '\n'
		res += '    Material Flags: ' + str(self.material_flags) + '\n'
		res += '    Material Properties: {\n'
		for prop in self.material_properties:
			res += '        ' + str(prop) + '\n'
		res += '    }\n'
		res += '    Fragment Fields: {\n'
		for field in self.fragment_fields:
			res += '        ' + str(field) + '\n'
		res += '    }\n'
		res += '    Shading State Size: ' + str(self.shading_state_size) + '\n'
		res += '}'
		return res


def evaluate_integer_statement(src_line: source_line, err: str = 'Failed to evaluate') -> int:
	try:
		tokens = tokenize_c_like(src_line.processed_text)
		text = ''
		for tok in tokens:
			if tok == ';':
				break
			text += tok
		return evaluate_statement(text)
	except:
		raise Exception(err + ' [' + str(src_line) + ']')


def parse_lit_shader(src_cache: source_cache, jls_path: str) -> lit_shader_data:
	shader_name = os.path.splitext(os.path.basename(jls_path))[0]
	default_editor_path = material_path(
		'"' + shader_name + '"', 
		'"' + jls_path + '"', 
		'"' + shader_name + ' at ' + jls_path + '"')
	result = lit_shader_data(default_editor_path)
	preprocessor = preporocessor_state(src_cache)

	for opt in blend_mode_options:
		preprocessor.macro_definitions[opt] = macro_definition(opt, {}, [str(blend_mode_options[opt])])
	for opt in material_flags:
		preprocessor.macro_definitions[opt] = macro_definition(opt, {}, [str(material_flags[opt])])


	def process_materialPath_pragma(args: source_line):
		path_tokens = extract_syntax_tree_from_source_line(args)
		for token in path_tokens:
			if not token.has_child_nodes() or token.token.token != '(':
				continue
			editor_path = material_path('', '', '')
			def add_path():
				if len(editor_path.name + editor_path.path + editor_path.hint) <= 0:
					return
				result.editor_paths.append(material_path(
					editor_path.name if (len(editor_path.name) > 0) else default_editor_path.name,
					editor_path.path if (len(editor_path.path) > 0) else default_editor_path.path,
					editor_path.hint if (len(editor_path.hint) > 0) else default_editor_path.hint))
				editor_path.name = ''
				editor_path.path = ''
				editor_path.hint = ''
				# print('Material Path: ' + str(result.editor_paths[-1]))
			for i in range(1, len(token.child_nodes) - 1):
				if token.child_nodes[i].token.token != '=':
					continue
				elif len(token.child_nodes[i + 1].token.token) <= 0 or token.child_nodes[i + 1].token.token[0] != '"':
					print(
						JM_MaterialPath_pragma_name + ' SHOULD TO LOOK LIKE: ' +
						'"' + JM_MaterialPath_pragma_name + '(name = "N" /* Optional */, path = "A/B/N" /* Optional */, hint: "Info" /* Optional */)"! '
						'Encountered "=" without proper string value on the right; ignoring the occurence. ' + str(args))
				elif token.child_nodes[i - 1].token.token == 'name':
					editor_path.name = token.child_nodes[i + 1].token.token
				elif token.child_nodes[i - 1].token.token == 'path':
					editor_path.path = token.child_nodes[i + 1].token.token
				elif token.child_nodes[i - 1].token.token == 'hint':
					editor_path.hint = token.child_nodes[i + 1].token.token
			add_path()
	preprocessor.pragma_handlers[JM_MaterialPath_pragma_name] = process_materialPath_pragma


	def process_blend_mode_pragma(args: source_line):
		result.blend_mode = evaluate_integer_statement(args, 'Failed to evaluate blend mode!')
		# print('Blend Mode: ' + str(result.blend_mode))
	preprocessor.pragma_handlers[JM_BlendMode_pragma_name] = process_blend_mode_pragma


	def process_material_flags_pragma(args: source_line):
		result.material_flags = evaluate_integer_statement(args, 'Failed to evaluate material flags!')
		# print('Material Flags: ' + str(result.material_flags))
	preprocessor.pragma_handlers[JM_MaterialFlags_pragma_name] = process_material_flags_pragma


	def process_materialProperty_pragma(args: source_line):
		prop_tokens = extract_syntax_tree_from_source_line(args)
		next_property = material_property(None, '', '', '', '')
		def add_property():
			if (next_property.value_type is not None) and (len(next_property.variable_name) > 0):
				# TODO: Resolve default value properly...
				result.material_properties.append(material_property(
					next_property.value_type, next_property.variable_name,
					next_property.default_value if (len(next_property.default_value) > 0) else '0',
					next_property.editor_alias if (len(next_property.editor_alias) > 0) else next_property.variable_name,
					next_property.hint if (len(next_property.hint) > 0) else (next_property.value_type.cpp_name + ' ' + next_property.variable_name)))
				# print('Material Property: ' + str(result.material_properties[-1]))
			next_property.variable_name = ''
			next_property.default_value = ''
			next_property.editor_alias = ''
		known_typenames = built_in_primitive_typenames
		for token in prop_tokens:
			if token.has_child_nodes():
				for i in range(1, len(token.child_nodes) - 1):
					if token.child_nodes[i].token.token != '=':
						continue
					elif token.child_nodes[i - 1].token.token == 'alias':
						if len(token.child_nodes[i + 1].token.token) <= 0 or token.child_nodes[i + 1].token.token[0] != '"':
							print(JM_MaterialPath_pragma_name + ' alias should be a string! ignoring occurence(' + token.child_nodes[i + 1].token.token + '). ' + str(args))
						else:
							next_property.editor_alias = token.child_nodes[i + 1].token.token
					elif token.child_nodes[i - 1].token.token == 'hint':
						if len(token.child_nodes[i + 1].token.token) <= 0 or token.child_nodes[i + 1].token.token[0] != '"':
							print(JM_MaterialPath_pragma_name + ' hint should be a string! ignoring occurence(' + token.child_nodes[i + 1].token.token + '). ' + str(args))
						else:
							next_property.hint = token.child_nodes[i + 1].token.token
					elif token.child_nodes[i - 1].token.token == 'default':
						j = i + 1
						while j < len(token.child_nodes):
							if token.child_nodes[j] == ',' or token.child_nodes[j] == ';':
								break
							elif token.child_nodes[j] in known_typenames:
								j += 1
								continue
							next_property.default_value = token.child_nodes[j].to_str('', '', '')
							break
							
			elif token.end_bracket_token() is not None:
				continue
			elif next_property.value_type is None:
				if token.token.token in known_typenames:
					next_property.value_type = known_typenames[token.token.token]
				else:
					raise Exception(JM_MaterialPropety_pragma_name + ' - "' + token.token.token + '" can not be used as a type! ' + str(args))
			elif token.token.token == ',':
				add_property()
			elif token.token.token == ';':
				add_property()
				next_property.value_type = None
			elif len(token.token.token) > 0:
				if (not token.token.token[0].isnumeric()) and (token.token.token not in known_typenames):
					next_property.variable_name = token.token.token
					add_property()
				else:
					raise Exception(JM_MaterialPropety_pragma_name + ' - "' + token.token.token + '" can not be used as a property name! ' + str(args))
		add_property()
	preprocessor.pragma_handlers[JM_MaterialPropety_pragma_name] = process_materialProperty_pragma


	def process_fragmentField_pragma_name(args: source_line):
		field_tokens = extract_syntax_tree_from_source_line(args)
		next_field = fragment_field('', '')
		def add_field():
			if (len(next_field.typename) <= 0) or (len(next_field.variable_name) <= 0):
				return
			result.fragment_fields.append(fragment_field(next_field.typename, next_field.variable_name))
			next_field.variable_name = ''
			# print('Fragment Field: ' + str(result.fragment_fields[-1]))
		for token in field_tokens:
			if token.has_child_nodes() or token.end_bracket_token() is not None:
				continue
			elif len(token.token.token) <= 0:
				continue
			elif token.token.token == ',':
				add_field()
			elif token.token.token == ';':
				add_field()
				next_field.typename = ''
			elif token.token.token[0].isnumeric():
				raise Exception(JM_FragmentField_pragma_name + ' - Unexpected token: "' + token.token.token + '"! ' + str(args))
			elif len(next_field.typename) <= 0:
				next_field.typename = token.token.token
			else:
				next_field.variable_name = token.token.token
				add_field()
		add_field()
	preprocessor.pragma_handlers[JM_FragmentField_pragma_name] = process_fragmentField_pragma_name	


	def process_shadingStateSize_pragma_name(args: source_line):
		result.shading_state_size = evaluate_integer_statement(args, 'Failed to evaluate shading state size!')
		# print('Shading State Size: ' + str(result.shading_state_size))
	preprocessor.pragma_handlers[JM_ShadingStateSize_pragma_name] = process_shadingStateSize_pragma_name	


	preprocessor.include_file(jls_path)
	return result


if __name__ == "__main__":
	cache = source_cache([os.path.realpath(os.path.dirname(os.path.realpath(__file__)) + "/../../__Source__")])
	shader_data = parse_lit_shader(cache, "Jimara/Data/Materials/JLS_Template.jls.sample")
	print(shader_data)
	#raw_syntax_tree = syntax_tree_extractor.extract_syntax_tree(tokens)[0]
	#syntax_tree = unify_namespace_paths(raw_syntax_tree)
	#for node in syntax_tree:
	#	print(node.to_str('  '))
