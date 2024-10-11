import sys, os
if __name__ == "__main__":
	sys.path.insert(0, os.path.realpath(os.path.dirname(os.path.realpath(__file__)) + "/../"))
import jimara_file_tools, code_analysis.jimara_tokenize_source as jimara_tokenize_source

instructions = (
			"Usage: python light_definition_processor.py source_directory glsl_output_file cpp_output_file <light_type_info> <extensions...>\n" +
			"    source_directory - Light shaders will be searched in this directory, as well as it's subfolders;\n" +
			"    glsl_output_file - Merged light shader code will be stored in this \"output\" header file containing Jimara_GetLightSamples function;\n" +
			"    cpp_output_file  - glsl_output_file will depend on \"Light Type identifiers\" to decide which light shader to run.\n" +
			"                       Those identifiers will be stored in this header as <light_type_info> unordered_map;\n" + 
			"    light_type_info  - Optional name for the information container stored in cpp_output_file (defaults to JIMARA_LIGHT_TYPE_INFO);\n" + 
			"    extensions       - Optional list of light shader extensions to find the shader files in source_directory (defaults to \"jld\"<stands for \"Jimara Light Definition\">).")


def get_type_names(shader_paths):
	return jimara_file_tools.strip_file_extensions(jimara_file_tools.get_file_names(shader_paths))


def merge_light_shaders(shader_paths):
	code = (
		"// Bindless samplers and buffers:\n" +
		"#ifndef BINDLESS_SAMPLER_SET_ID\n" +
		"#define BINDLESS_SAMPLER_SET_ID 0\n" +
		"layout (set = BINDLESS_SAMPLER_SET_ID, binding = 0) uniform sampler2D jimara_BindlessTextures[];\n" + 
		"#endif\n" + 
		"#ifndef BINDLESS_BUFFER_SET_ID\n" +
		"#define BINDLESS_BUFFER_SET_ID (BINDLESS_SAMPLER_SET_ID + 1)\n" +
		"layout (set = BINDLESS_BUFFER_SET_ID, binding = 0) buffer Jimara_BindlessBuffers { uint data[]; } jimara_BindlessBuffers[];\n" +
		"#endif\n\n" + 

		"// COMMON PARAMETERS:\n" +
		"#ifndef LIGHT_BINDING_SET_ID\n" + 
		"#define LIGHT_BINDING_SET_ID (BINDLESS_BUFFER_SET_ID + 1)\n" + 
		"#endif\n" + 
		"#ifndef LIGHT_BINDING_START_ID\n" + 
		"#define LIGHT_BINDING_START_ID 0\n" + 
		"#endif\n\n" + 

		"// ILLUMINATED POINT DEFINITION:\n" + 
		"#ifndef HIT_POINT_INFO_DEFINED\n" +
		"struct HitPoint {\n" + 
		"	vec3 position;\n" + 
		"	vec3 normal;\n" + 
		"	float roughness;\n" + 
		"};\n" +
		"#define HIT_POINT_INFO_DEFINED\n" +
		"#endif\n\n")

	type_names = get_type_names(shader_paths)

	buffer_attachment_name = lambda type_name: type_name + "_lightBuffer"
	light_data_type = lambda type_name: type_name + "_Data"
	light_type_id_name = lambda type_name: type_name + "_LightTypeId"
	light_function_name = lambda type_name: type_name + "_GetSamples"

	code += "\n// TYPE ID-s:\n"
	for i, type_name in enumerate(type_names):
		code += "#define " + light_type_id_name(type_name) + " " + str(i) + "\n"

	light_binding_stride = 0
	data_sizes = []

	code += "\n\n// ___________________________________________________________________________________________\n\n"
	for i, file in enumerate(shader_paths):
		with open(file, "r") as fl:
			source = fl.read()
		code += "// LIGHT TYPE: " + type_names[i] + " <SOURCE: \"" + file + "\">\n" + source + '\n'
		tokens = jimara_tokenize_source.tokenize_c_like(source)
		token_index = 0
		last_pragme_token_index = len(tokens) - 2
		data_size = None
		while token_index < last_pragme_token_index:
			token = tokens[token_index]
			token_index += 1
			if token == "#pragma":
				if tokens[token_index] == 'jimara_light_descriptor_size':
					try:
						data_size = int(tokens[token_index + 1])
						token_index += 2
					except:
						data_size = None
						print("Warning: " + repr(file) + " - jimara_light_descriptor_size should be a valid integer")
		if data_size is None:
			print("Error: Light descriptor " + repr(file) + " does not contain definition for jimara_light_descriptor_size (expected: #pragma jimara_light_descriptor_size num_bytes)")
			exit(1)
		if light_binding_stride < data_size:
			light_binding_stride = data_size
		data_sizes.append(data_size)
	light_binding_stride = int(int((light_binding_stride + 15) / 16) * 16)

	code += (
		"// ___________________________________________________________________________________________\n\n" + 
		"// ATTACHMENTS:\n" +
		"layout(std430, set = LIGHT_BINDING_SET_ID, binding = LIGHT_BINDING_START_ID) buffer Jimara_LightDataBinding { float data[]; } jimara_LightDataBinding;\n\n")
	for i, type_name in enumerate(type_names):
		buffer_name = type_name + "_LightBuffer"
		elem_name = buffer_name + "Elem"
		data_size = data_sizes[i]
		if data_size >= light_binding_stride:
			padding = ""
		else:
			padding = " float padding[" + str(int((light_binding_stride - data_size) / 4)) + "];"
		code += (
			"struct " + elem_name + " { " + light_data_type(type_name) + " data;" + padding + " };\n"
			"layout(std430, set = LIGHT_BINDING_SET_ID, binding = LIGHT_BINDING_START_ID) buffer " + buffer_name + 
			" { " + elem_name + " " + buffer_attachment_name(type_name) + "[]; };\n\n")

	code += (
		"\n// Computes sample photons coming to the hit point from light defined by light buffer index and light type index\n" +
		"#define Jimara_IterateLightSamples(JILS_lightBufferId, JILS_lightTypeId, JILS_hitPoint, JILS_RecordSampleFn) { \\\n")
	def search_type(types, tab):
		if len(types) < 1:
			return ""
		elif len(types) == 1:
			return tab + light_function_name(types[0]) + "(JILS_hitPoint, " + buffer_attachment_name(types[0]) + "[JILS_lightBufferId].data, JILS_RecordSampleFn); \\\n"
		else:
			return (
				tab + "[[branch]] if (JILS_lightTypeId < " + light_type_id_name(types[int(len(types)/2)]) + ") { \\\n" +
				search_type(types[:int(len(types)/2)], tab + "\t") + 
				tab + "} else { \\\n" + 
				search_type(types[int(len(types)/2):], tab + "\t") +
				tab + "} \\\n")
	code += (
		search_type(type_names, "\t") + 
		"}\n\n")

	code += (
		"#define JIMARA_GetLightSamples_FN Jimara_IterateLightSamples\n" +
		"#define LIGHT_BINDING_END_ID (LIGHT_BINDING_START_ID + 1)\n")

	return code, light_binding_stride


def generate_engine_type_indices(shader_paths, storage_name, light_elem_size, namespaces = ["Jimara", "LightRegistry"], inset = "", generate_includes = True):
	if generate_includes:
		code = (
			inset + "#include <unordered_map>\n" + 
			inset + "#include <cstdint>\n" +
			inset + "#include <string>\n\n")
	else:
		code = ""

	if len(namespaces) > 0:
		return (
			code + inset + "namespace " + namespaces[0] + " {\n" + 
			generate_engine_type_indices(shader_paths, storage_name, light_elem_size, namespaces[1:], inset + "\t", False) + inset + "}\n")

	escape = lambda type_name: repr(type_name)[1:(len(repr(type_name)) - 1)]

	code += (
		inset + "static const struct {\n" + 
		inset + "\tconst std::unordered_map<std::string, uint32_t> typeIds;\n" +
		inset + "\tconst std::size_t perLightDataSize;\n" + 
		inset + "} " + storage_name + " = {\n" +
		inset + "\tstd::unordered_map<std::string, uint32_t>({")
	for i, type_name in enumerate(get_type_names(shader_paths)):
		code += ("\n" if i <= 0 else ",\n") + inset + "\t\t{ \"" + escape(type_name) + "\", " + str(i) + " }"
	code += (
		"\n" + inset + "\t}),\n" + 
		inset + "\t" + str(light_elem_size) + "\n" + 
		inset + "};\n")

	return code


if __name__ == "__main__":
	if len(sys.argv) < 4:
		print(instructions)
		exit()
	else:
		source_directory = sys.argv[1]
		glsl_output_file = sys.argv[2]
		cpp_output_file = sys.argv[3]
		type_id_map = "JIMARA_LIGHT_TYPE_INFO" if len(sys.argv) <= 4 else sys.argv[4]
		light_extensions = ["jld"] if len(sys.argv) <= 5 else sys.argv[5:]
		paths = jimara_file_tools.find_by_extension(source_directory, light_extensions)
		code, elem_size = merge_light_shaders(paths)
		with open(glsl_output_file, "w") as file:
			file.write(code)
		with open(cpp_output_file, "w") as file:
			file.write(generate_engine_type_indices(paths, type_id_map, elem_size))
