import sys, os
if __name__ == "__main__":
	sys.path.insert(0, os.path.realpath(os.path.dirname(os.path.realpath(__file__)) + "/../"))
from code_analysis.source_cache import source_cache
import lit_shader_compilation.lit_shader_processor as lit_shaders 
import lit_shader_compilation.lighting_model_processor as lighting_models 
from code_analysis.preprocessor import generate_include_statement

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


if __name__ == "__main__":
	cache = source_cache([os.path.realpath(os.path.dirname(os.path.realpath(__file__)) + "/../../__Source__")])
	shader_data = lit_shaders.parse_lit_shader(cache, "Jimara/Data/Materials/JLS_Template.jls.sample")
	model_data = lighting_models.parse_lighting_model(cache, "Jimara/Environment/Rendering/LightingModels/JLM_Template.jlm.sample")
	print(generate_glsl_source(shader_data, model_data, "JM_LightDefinitions.glh"))

