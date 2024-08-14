import sys, os
if __name__ == "__main__":
	sys.path.insert(0, os.path.realpath(os.path.dirname(os.path.realpath(__file__)) + "/../"))
from code_analysis.source_cache import source_cache
from code_analysis.preprocessor import preporocessor_state, source_line, macro_definition


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


blend_mode_options: dict[str, int]
blend_mode_options = {
	'JM_Blend_Opaque':		0,
	'JM_Blend_Alpha':		1,
	'JM_Blend_Additive':	2
}


material_flags: dict[str, int]
material_flags = {
	'JM_CanDiscard':					(1 << 0),
	'JM_UseObjectId':					(1 << 1),
	'JM_UsePerVertexTilingAndOffset':	(1 << 2),
	'JM_UseVertexColor':				(1 << 3)
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
		self.material_properties: list[material_property]
		self.material_properties = []
		self.fragment_fields: list[fragment_field]
		self.fragment_fields = []


def parse_lit_shader(src_cache: source_cache, jls_path: str) -> lit_shader_data:
	result = lit_shader_data()
	preprocessor = preporocessor_state(src_cache)

	for opt in blend_mode_options:
		preprocessor.macro_definitions[opt] = macro_definition(opt, {}, [str(blend_mode_options[opt])])
	for opt in material_flags:
		preprocessor.macro_definitions[opt] = macro_definition(opt, {}, [str(material_flags[opt])])

	def process_blend_mode_pragma(args: source_line):
		assert(isinstance(args, source_line))
		print(JM_BlendMode_pragma_name + " " + args.processed_text)
	preprocessor.pragma_handlers[JM_BlendMode_pragma_name] = process_blend_mode_pragma

	def process_material_flags_pragma(args: source_line):
		assert(isinstance(args, source_line))
		print(JM_MaterialFlags_pragma_name + " " + args.processed_text)
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
