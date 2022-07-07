import json, hashlib, base64

class jimara_light_type_ids:
    def __init__(self) -> None:
        self.__light_type_ids = []
        self.__per_light_data_size = 0
        self.__lighting_model_paths = {}
        self.get_general_shader_directory()

    def add_light_type(self, name : str, data_size : int) -> None:
        self.__light_type_ids.append(name)
        if (data_size > self.__per_light_data_size):
            self.__per_light_data_size = data_size

    def get_general_shader_directory(self) -> str:
        return self.get_lighting_model_directory("")

    def get_lighting_model_directory(self, local_path: str) -> str:
        if local_path in self.__lighting_model_paths:
            return self.__lighting_model_paths[local_path]
        hash = base64.urlsafe_b64encode(hashlib.sha256(local_path.encode('utf-8')).digest()).decode('utf-8').replace('=', '')
        self.__lighting_model_paths[local_path] = hash
        return hash

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
        rv += ( 
            indent + "\t}\n" +
            indent + "}\n")
        return rv

    def __str__(self) -> str:
        return self.as_str("")

class jimara_shader_data:
    def __init__(self) -> None:
        self.light_types = jimara_light_type_ids();

    def __str__(self) -> str:
        rv = "{\n"
        rv += "\t\"LightTypeIds\": " + self.light_types.as_str('\t')
        rv += "}"
        return rv
