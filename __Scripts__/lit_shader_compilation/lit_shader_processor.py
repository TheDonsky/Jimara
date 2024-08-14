import sys, os
if __name__ == "__main__":
	sys.path.insert(0, os.path.realpath(os.path.dirname(os.path.realpath(__file__)) + "/../"))
from code_analysis.source_cache import source_cache
from code_analysis.preprocessor import preporocessor_state, source_line, macro_definition, evaluate_statement


JM_BlendMode_pragma_name = "JM_BlendMode"
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
	'mat4':		type_info('mat4', '::Jimara::Matrix4', 64, 16)
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


class material_property:
	def __init__(self, value_type: type_info, variable_name: str, default_value: str, editor_alias: str, hint: str) -> None:
		self.value_type = value_type
		self.variable_name = variable_name
		self.default_value = default_value
		self.editor_alias = self.self.variable_name if (editor_alias is None) else editor_alias
		self.hint = (self.self.variable_name + ' [' + self.typename + ']') if (hint is None) else hint


class fragment_field:
	def __init__(self, typename: str, variable_name: str) -> None:
		self.typename = typename
		self.variable_name = variable_name


class lit_shader_data:
	def __init__(self) -> None:
		self.blend_mode = JM_Blend_Opaque_name
		self.material_flags = 0
		self.material_properties: list[material_property]
		self.material_properties = []
		self.fragment_fields: list[fragment_field]
		self.fragment_fields = []
		self.shading_state_size = 0


def parse_lit_shader(src_cache: source_cache, jls_path: str) -> lit_shader_data:
	result = lit_shader_data()
	preprocessor = preporocessor_state(src_cache)

	for opt in blend_mode_options:
		preprocessor.macro_definitions[opt] = macro_definition(opt, {}, [str(blend_mode_options[opt])])
	for opt in material_flags:
		preprocessor.macro_definitions[opt] = macro_definition(opt, {}, [str(material_flags[opt])])

	def process_blend_mode_pragma(args: source_line):
		assert(isinstance(args, source_line))
		try:
			result.blend_mode = evaluate_statement(args.processed_text)
			print('Blend Mode: ' + str(result.blend_mode))
		except:
			print('Failed to evaluate blend mode: ' + repr(args.processed_text) + '! ' + str(args))
			assert(False)
	preprocessor.pragma_handlers[JM_BlendMode_pragma_name] = process_blend_mode_pragma

	def process_material_flags_pragma(args: source_line):
		assert(isinstance(args, source_line))
		try:
			result.material_flags = evaluate_statement(args.processed_text)
			print('Material Flags: ' + str(result.material_flags))
		except:
			print('Failed to evaluate material flags: ' + repr(args.processed_text) + '!' + str(args))
			assert(False)
	preprocessor.pragma_handlers[JM_MaterialFlags_pragma_name] = process_material_flags_pragma

	def process_materialProperty_pragma(args: source_line):
		assert(isinstance(args, source_line))
		print(JM_MaterialPropety_pragma_name + " " + args.processed_text)
	preprocessor.pragma_handlers[JM_MaterialPropety_pragma_name] = process_materialProperty_pragma

	def process_fragmentField_pragma_name(args: source_line):
		assert(isinstance(args, source_line))
		print(JM_FragmentField_pragma_name + " " + args.processed_text)
	preprocessor.pragma_handlers[JM_FragmentField_pragma_name] = process_fragmentField_pragma_name	

	def process_shadingStateSize_pragma_name(args: source_line):
		assert(isinstance(args, source_line))
		print(JM_ShadingStateSize_pragma_name + " " + args.processed_text)
	preprocessor.pragma_handlers[JM_ShadingStateSize_pragma_name] = process_shadingStateSize_pragma_name	

	preprocessor.include_file(jls_path)
	return result


if __name__ == "__main__":
	cache = source_cache([os.path.realpath(os.path.dirname(os.path.realpath(__file__)) + "/../../__Source__")])
	parse_lit_shader(cache, "Jimara/Data/Materials/JLS_Template.jls.sample")
	#raw_syntax_tree = syntax_tree_extractor.extract_syntax_tree(tokens)[0]
	#syntax_tree = unify_namespace_paths(raw_syntax_tree)
	#for node in syntax_tree:
	#	print(node.to_str('  '))
