import json, hashlib, base64, os
from lit_shader_compilation import lit_shader_processor
from lit_shader_compilation import lit_shader_compilation_utils


class jimara_shader_data:
	def __init__(self) -> None:
		self.__light_type_ids = []
		self.__per_light_data_size = 0
		self.__lighting_model_paths = {}
		self.__used_lighting_model_keys = {}
		self.__lit_shader_records: dict[str, lit_shader_processor.lit_shader_data] = {}
		self.get_general_shader_directory()

	def add_light_type(self, name : str, data_size : int) -> None:
		self.__light_type_ids.append(name)
		if (data_size > self.__per_light_data_size):
			self.__per_light_data_size = data_size

	def add_lit_shader(self, shader: lit_shader_processor.lit_shader_data) -> None:
		if shader is None:
			return
		assert((not shader.path.path in self.__lit_shader_records) or self.__lit_shader_records[shader.path.path] is shader)
		self.__lit_shader_records[shader.path.path] = shader

	def get_general_shader_directory(self) -> str:
		return self.get_lighting_model_directory("")

	def get_lighting_model_directory(self, local_path: str) -> str:
		local_path = local_path.replace('\\', '/')
		if local_path in self.__lighting_model_paths:
			return self.__lighting_model_paths[local_path]
		hash = os.path.splitext(os.path.basename(local_path))[0]
		if len(hash) > 8:
			hash = base64.urlsafe_b64encode(hashlib.sha256(local_path.encode('utf-8')).digest()).decode('utf-8').replace('=', '')[:8]
		key = hash
		index = 0
		while key in self.__used_lighting_model_keys:
			index += 1
			key = hash + "_" + str(index)
		self.__lighting_model_paths[local_path] = key
		self.__used_lighting_model_keys[key] = local_path
		return key

	def as_str(self, indent : str) -> str:
		rv = "{\n" + indent + "\t\"LightTypes\": {\n"
		for i, type_name in enumerate(self.__light_type_ids):
			rv += indent + "\t\t" + json.dumps(type_name) + ": " + i.__str__() + ("\n" if (i >= (len(self.__light_type_ids) - 1)) else ",\n")
		rv += (
			indent + "\t},\n" +
			indent + "\t\"PerLightDataSize\": " + self.__per_light_data_size.__str__() + ",\n" + 
			indent + "\t\"LightingModels\": {\n")
		keys = self.__lighting_model_paths.keys()
		for i, path in enumerate(keys):
			rv += indent + '\t\t' + json.dumps(path) + ": " + json.dumps(self.__lighting_model_paths[path]) + ("\n" if (i >= (len(keys) - 1)) else ",\n")
		rv += indent + "\t},\n"
		rv += indent + '\t"LitShaders": {'
		shaders = [shader for shader in self.__lit_shader_records.values()]
		if len(shaders) > 0:
			rv += '\n' + indent + '\t\t' + '"Count": ' + str(len(shaders))
			for i in range(len(shaders)):
				rv += ',\n' + indent + '\t\t' + json.dumps(str(i)) + ': '
				rv += lit_shader_compilation_utils.generate_lit_shader_definition_json(shaders[i], '\t\t', '\t', '\n')
			rv += '\n' + indent + "\t"
		rv += "}\n"
		rv += indent + "}\n"
		return rv

	def __str__(self) -> str:
		return self.as_str("")
