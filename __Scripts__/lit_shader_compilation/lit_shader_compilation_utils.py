import sys, os
if __name__ == "__main__":
	sys.path.insert(0, os.path.realpath(os.path.dirname(os.path.realpath(__file__)) + "/../"))
from code_analysis.source_cache import source_cache
import lit_shader_compilation.lit_shader_processor as lit_shaders 
import lit_shader_compilation.lighting_model_processor as lighting_models 
from code_analysis.preprocessor import generate_include_statement, source_path_repr
import json

def generate_glsl_source(
		lit_shader: lit_shaders.lit_shader_data,
		lighting_model: lighting_models.lighting_model_data,
		light_definition_relative_path: str) -> str:
	return (
		"#version 460\n\n\n\n\n" +
		"/**\n" + 
		"################################################################################\n" +
		"######################### LIGHT TYPES AND DEFINITIONS: #########################\n" + 
		"*/\n" +
		generate_include_statement(light_definition_relative_path) + "\n\n\n\n\n" +
		"/**\n" + 
		"################################################################################\n" +
		"################################ SHADER SOURCE: ################################\n" + 
		"*/\n" +
		"#define MATERIAL_BINDING_SET_ID (LIGHT_BINDING_SET_ID + 1)\n" +
		lit_shader.generate_glsl_header() + '\n' +
		generate_include_statement(lit_shader.jls_path) + "\n\n\n\n\n" +
		"/**\n" + 
		"################################################################################\n" +
		"############################ LIGHTING MODEL SOURCE: ############################\n" + 
		"*/\n" +
		"#define MODEL_BINDING_SET_ID LIGHT_BINDING_SET_ID\n"
		"#define MODEL_BINDING_START_ID LIGHT_BINDING_END_ID\n" +
		lighting_models.generate_shader_stage_macro_definitions() + '\n' +
		generate_include_statement(lighting_model.jlm_path) + "\n")


def generate_lit_shader_definition_json(lit_shader: lit_shaders.lit_shader_data, inset = '', tab = '\t', endline = '\n') -> str:
	res = '{' + endline
	res += inset + tab + '"Shader Path": ' + source_path_repr(os.path.splitext(lit_shader.path.path)[0]) + ',' + endline
	res += inset + tab + '"Editor Paths": {'
	if len(lit_shader.editor_paths) > 0:
		res += endline + inset + tab + tab + '"Size": ' + str(len(lit_shader.editor_paths))
		for i in range(len(lit_shader.editor_paths)):
			path = lit_shader.editor_paths[i]
			res += ',' + endline + inset + tab + tab + '"' + str(i) + '": {' + endline
			res += inset + tab + tab + tab + '"Name": ' + json.dumps(path.name) + ',' + endline
			res += inset + tab + tab + tab + '"Path": ' + source_path_repr(path.path) + ',' + endline
			res += inset + tab + tab + tab + '"Hint": ' + json.dumps(path.hint) + endline
			res += inset + tab + tab + '}'
		res += endline + inset + tab
	res += '},' + endline
	res += inset + tab + '"Blend Mode": ' + str(lit_shader.blend_mode) + ',' + endline
	res += inset + tab + '"Material Flags": ' + str(lit_shader.material_flags) + ',' + endline
	res += inset + tab + '"Material Properties": {'
	if (len(lit_shader.material_properties)) > 0:
		res += endline + inset + tab + tab + '"Size": ' + str(len(lit_shader.material_properties))
		for i in range(len(lit_shader.material_properties)):
			prop = lit_shader.material_properties[i]
			res += ',' + endline + inset + tab + tab + '"' + str(i) + '": {' + endline
			res += inset + tab + tab + tab + '"Type": ' + json.dumps(prop.value_type.enum_id) + ',' + endline
			res += inset + tab + tab + tab + '"Name": ' + json.dumps(prop.variable_name) + ',' + endline
			res += inset + tab + tab + tab + '"Alias": ' + json.dumps(prop.editor_alias) + ',' + endline
			res += inset + tab + tab + tab + '"Hint": ' + json.dumps(prop.hint) + ',' + endline
			res += inset + tab + tab + tab + '"Default Value": ' + json.dumps(prop.default_value)
			attribute_count = 0 if prop.attributes is None else len(prop.attributes)
			if attribute_count > 0:
				res += ',' + endline + inset + tab + tab + tab + '"Attributes": {' + endline
				res += inset + tab + tab + tab + tab + '"Size": ' + str(attribute_count)
				for attrId in range(attribute_count):
					attr = prop.attributes[attrId]
					res += ',' +endline + inset + tab + tab + tab + tab + '"' + str(attrId) + '": { "Type": ' + json.dumps(attr.type_id) + ', "Value": ' + json.dumps(attr.value) + "}"
				res += endline + inset + tab + tab + tab + '}'
			res += endline + inset + tab + tab + '}'
		res += endline + inset + tab
	res += '},' + endline
	res += inset + tab + '"Shading State Size": ' + str(lit_shader.shading_state_size) + endline
	res += inset + '}'
	return res 


if __name__ == "__main__":
	cache = source_cache([os.path.realpath(os.path.dirname(os.path.realpath(__file__)) + "/../../__Source__")])
	shader_data = lit_shaders.parse_lit_shader(cache, "Jimara/Data/Materials/JLS_Template.jls.sample")
	model_data = lighting_models.parse_lighting_model(cache, "Jimara/Environment/Rendering/LightingModels/JLM_Template.jlm.sample")
	print(generate_glsl_source(shader_data, model_data, "JM_LightDefinitions.glh"))
	print("______________________________________________________________________")
	print(generate_lit_shader_definition_json(shader_data))

