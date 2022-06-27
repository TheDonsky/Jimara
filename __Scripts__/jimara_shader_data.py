import json

class jimara_light_type_ids:
    def __init__(self) -> None:
        self.__light_type_ids = []
        self.__per_light_data_size = 0

    def add_light_type(self, name : str, data_size : int) -> None:
        self.__light_type_ids.append(name)
        if (data_size > self.__per_light_data_size):
            self.__per_light_data_size = data_size

    def as_str(self, indent : str) -> str:
        rv = "{\n" + indent + "\t\"LightTypes\": {\n"
        for i, type_name in enumerate(self.__light_type_ids):
            rv += indent + "\t\t" + json.dumps(type_name) + ": " + i.__str__() + ("\n" if (i >= (len(self.__light_type_ids) - 1)) else ",\n")
        rv += (
            indent + "\t},\n" +
            indent + "\t\"PerLightDataSize\": " + self.__per_light_data_size.__str__() + "\n" + 
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
