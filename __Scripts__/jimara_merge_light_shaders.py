import jimara_file_tools, sys


def get_type_names(shader_paths):
	return jimara_file_tools.strip_file_extensions(jimara_file_tools.get_file_names(shader_paths))


def merge_light_shaders(shader_paths):
	code = (
		"// COMMON PARAMETERS:\n"
		"#ifndef MAX_PER_LIGHT_SAMPLES\n" + 
		"#define MAX_PER_LIGHT_SAMPLES 8\n" + 
		"#endif\n" + 
		"#ifndef LIGHT_BINDING_SET_ID\n" + 
		"#define LIGHT_BINDING_SET_ID 0\n" + 
		"#endif\n" + 
		"#ifndef LIGHT_BINDING_START_ID\n" + 
		"#define LIGHT_BINDING_START_ID 0\n" + 
		"#endif\n\n" + 
		"// ILLUMINATED POINT DEFINITION:\n" + 
		"#ifndef HIT_POINT_INFO_DEFINED\n" +
		"struct HitPoint { vec3 position; vec3 normal; };\n" +
		"#define HIT_POINT_INFO_DEFINED\n" +
		"#endif\n\n" +
		"// PHOTON DEFINITION:\n"
		"#ifndef PHOTON_DEFINED\n" +
		"struct Photon { vec3 origin; vec3 color; };\n" +
		"#define PHOTON_DEFINED\n" +
		"#endif\n\n")

	type_names = get_type_names(shader_paths)

	buffer_attachment_name = lambda type_name: type_name + "_lightBuffer"
	light_data_type = lambda type_name: type_name + "_Data"
	light_type_id_name = lambda type_name: type_name + "_LightTypeId"
	light_function_name = lambda type_name: type_name + "_GetSamples"

	code += "\n// TYPE ID-s:\n"
	for i, type_name in enumerate(type_names):
		code += "#define " + light_type_id_name(type_name) + " " + str(i) + "\n"

	code += "// ___________________________________________________________________________________________\n\n"
	for i, file in enumerate(shader_paths):
		with open(file, "r") as fl:
			code += "// LIGHT TYPE: " + type_names[i] + " <SOURCE: \"" + file + "\">\n" + fl.read() + "\n"

	code += (
		"// ___________________________________________________________________________________________\n\n" + 
		"// ATTACHMENTS:\n")
	for type_name in type_names:
		code += (
			"layout(std430, set = LIGHT_BINDING_SET_ID, binding = LIGHT_BINDING_START_ID) buffer " + type_name + "_LightBuffer { " 
			+ light_data_type(type_name) + " " + buffer_attachment_name(type_name) + "[]; };\n")

	code += (
		"\n// Computes sample photons coming to the hit point from light defined by light buffer index and light type index\n" +
		"uint Jimara_GetLightSamples(uint lightBufferId, uint lightTypeId, in HitPoint hitPoint, out Photon samples[MAX_PER_LIGHT_SAMPLES]) {\n")
	def search_type(types, tab):
		if len(types) < 1:
			return tab + "return 0;\n"
		elif len(types) == 1:
			return tab + "return " + light_function_name(types[0]) + "(hitPoint, " + buffer_attachment_name(types[0]) + "[lightBufferId], samples);\n"
		else:
			return (
				tab + "if (lightTypeId < " + light_type_id_name(types[int(len(types)/2)]) + ") {\n" +
				search_type(types[:int(len(types)/2)], tab + "\t") + 
				tab + "} else {\n" + 
				search_type(types[int(len(types)/2):], tab + "\t") +
				tab + "}\n")
	code += search_type(type_names, "\t")
	code += (
		"}\n" + 
		"#define JIMARA_GetLightSamples_FN Jimara_GetLightSamples\n" +
		"#define LIGHT_BINDING_END_ID (LIGHT_BINDING_START_ID + 1)\n")

	return code


def generate_engine_type_indices(shader_paths, set_name, namespaces = ["Jimara", "LightRegistry"], inset = "", generate_includes = True):
	if generate_includes:
		code = (
			inset + "#include <unordered_map>\n" + 
			inset + "#include <string>\n\n")
	else:
		code = ""

	if len(namespaces) > 0:
		return code + inset + "namespace " + namespaces[0] + " {\n" + generate_engine_type_indices(shader_paths, set_name, namespaces[1:], inset + "\t", False) + inset + "}\n"

	escape = lambda type_name: repr(type_name)[1:(len(repr(type_name)) - 1)]

	code += (
		inset + "typedef unsigned int LightTypeId;\n" +
		inset + "static const std::unordered_map<std::string, LightTypeId> " + set_name + " = {")
	for i, type_name in enumerate(get_type_names(shader_paths)):
		code += ("\n" if i <= 0 else ",\n") + inset + "\t{ \"" + escape(type_name) + "\", " + str(i) + " }"
	code += "\n" + inset + "};\n"

	return code


if __name__ == "__main__":
	if len(sys.argv) < 4:
		print(
			"Usage: python jimara_merge_light_shaders.py source_directory glsl_output_file cpp_output_file <type_id_map> <extensions...>\n" +
			"    source_directory - Light shaders will be searched in this directory, as well as it's subfolders;\n" +
			"    glsl_output_file - Merged light shader code will be stored in this \"output\" header file containing Jimara_GetLightSamples function;\n" +
			"    cpp_output_file  - glsl_output_file will depend on \"Light Type identifiers\" to decide which light shader to run.\n" +
			"                       Those identifiers will be stored in this header as <type_id_map> unordered_map;\n" + 
			"    type_id_map      - Optional name for the unordered_map stored in cpp_output_file (defaults to JIMARA_LIGHT_TYPE_IDS);\n" + 
			"    extensions       - Optional list of light shader extensions to find the shader files in source_directory (defaults to \"jls\"<stands for \"Jimara Light Shader\">).")
	else:
		source_directory = sys.argv[1]
		glsl_output_file = sys.argv[2]
		cpp_output_file = sys.argv[3]
		type_id_map = "JIMARA_LIGHT_TYPE_IDS" if len(sys.argv) <= 4 else sys.argv[4]
		light_extensions = ["jls"] if len(sys.argv) <= 5 else sys.argv[5:]
		paths = jimara_file_tools.find_by_extension(source_directory, light_extensions)
		with open(glsl_output_file, "w") as file:
			file.write(merge_light_shaders(paths))
		with open(cpp_output_file, "w") as file:
			file.write(generate_engine_type_indices(paths, type_id_map))
