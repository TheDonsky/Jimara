import jimara_file_tools, sys


def get_type_names(shader_paths):
	return jimara_file_tools.strip_file_extensions(jimara_file_tools.get_file_names(shader_paths))


def light_type_id_name(type_name):
	return type_name + "_LightTypeId"


def merge_light_shaders(shader_paths):
	code = (
		"// COMMON PARAMETERS:\n"
		"#ifndef MAX_SAMPLES\n" + 
		"#define MAX_SAMPLES 8\n" + 
		"#endif\n" + 
		"#ifndef LIGHT_BUFFER_SET_ID\n" + 
		"#define LIGHT_BUFFER_SET_ID 0\n" + 
		"#endif\n" + 
		"#ifndef LIGHT_BUFFER_BINDING_ID\n" + 
		"#define LIGHT_BUFFER_BINDING_ID 0\n" + 
		"#endif\n\n" + 
		"// PHOTON DEFINITION:\n"
		"#ifndef PHOTON_DEFINED\n" +
		"struct Photon { vec3 origin; vec3 color };\n" +
		"#define PHOTON_DEFINED\n" +
		"#endif\n\n")

	type_names = get_type_names(shader_paths)

	buffer_attachment_name = lambda type_name: type_name + "_lightBuffer"
	light_data_type = lambda type_name: type_name + "_Data"
	light_function_name = lambda type_name: type_name + "_GetSamples"

	code += "\n// TYPE ID-s:\n"
	for i, type_name in enumerate(type_names):
		code += "#define " + light_type_id_name(type_name) + " " + str(i) + "\n"

	code += "___________________________________________________________________________________________\n\n"
	for i, file in enumerate(shader_paths):
		with open(file, "r") as fl:
			code += "// LIGHT TYPE: " + type_names[i] + " <SOURCE: \"" + file + "\">\n" + fl.read() + "\n"

	code += (
		"___________________________________________________________________________________________\n\n" + 
		"// ATTACHMENTS:\n")
	for type_name in type_names:
		code += (
			"layout(std430, set = LIGHT_BUFFER_SET_ID, binding = LIGHT_BUFFER_BINDING_ID) buffer " + type_name + "_LightBuffer { " 
			+ light_data_type(type_name) + " " + buffer_attachment_name(type_name) + "[]; };\n")

	code += (
		"\n// Computes sample photons coming to the hit point from light defined by light buffer index and light type index\n" +
		"uint Jimara_GetLightSamples(uint lightBufferId, uint lightTypeId, in vec3 hitPoint, in vec3 hitNormal, out Photon samples[MAX_SAMPLES]) {\n")
	def search_type(types, tab):
		if len(types) < 1:
			return
		elif len(types) == 1:
			return tab + "return " + light_function_name(types[0]) + "(hitPoint, hitNormal, " + buffer_attachment_name(types[0]) + "[lightBufferId], samples);\n"
		else:
			return (
				tab + "if (lightTypeId < " + light_type_id_name(types[int(len(types)/2)]) + ") {\n" +
				search_type(types[:int(len(types)/2)], tab + "\t") + 
				tab + "} else {\n" + 
				search_type(types[int(len(types)/2):], tab + "\t") +
				tab + "}\n")
	code += search_type(type_names, "\t")
	code += (
		"\treturn 0;\n" + 
		"}\n")

	return code


def generate_engine_type_indices(shader_paths, namespaces = ["Jimara", "LightRegistry"], inset = ""):
	if len(namespaces) > 0:
		return inset + "namespace " + namespaces[0] + " {\n" + generate_engine_type_indices(shader_paths, namespaces[1:], inset + "\t") + inset + "}\n"

	code = inset + "typedef unsigned int LightTypeId;\n"
	for i, type_name in enumerate(get_type_names(shader_paths)):
		code += inset + "const LightTypeId " + light_type_id_name(type_name) + " = " + str(i) + ";\n"

	return code


if __name__ == "__main__":
	source_dir = "." if len(sys.argv) <= 1 else sys.argv[1]
	glsl_output_file = "Lights.lights" if len(sys.argv) <= 2 else sys.argv[2]
	cpp_output_file = "LightTypes.cpp" if len(sys.argv) <= 3 else sys.argv[3]
	light_extensions = [".light"] if len(sys.argv) <= 4 else sys.argv[4:]
	paths = jimara_file_tools.find_by_suffix(source_dir, light_extensions)
	with open(glsl_output_file, "w") as file:
		file.write(merge_light_shaders(paths))
	with open(cpp_output_file, "w") as file:
		file.write(generate_engine_type_indices(paths))
