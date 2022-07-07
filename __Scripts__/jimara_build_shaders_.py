import os, sys, json
import jimara_file_tools
import jimara_shader_data
import jimara_source_dependencies
import jimara_merge_light_shaders

ERROR_NOT_YET_IMPLEMENTED = 1
ERROR_MISSING_ARGUMENTS = 2
ERROR_LIGHT_PATH_EXPECTED_DIRTY = 3

class job_directories:
	def __init__(self, src_dirs: list = [], include_dirs: list = [], intermediate_dir: str = None, output_dir: str = None) -> None:
		self.src_dirs = src_dirs.copy()
		self.include_dirs = include_dirs.copy()
		self.intermediate_dir = intermediate_dir
		self.output_dir = output_dir

class job_extensions:
	default_light_definition = 'jld'
	default_lighting_model = 'jlm'
	default_lit_shader = 'jls'

	def __init__(self, light_definition: list = [], lighting_model: list = [], lit_shader: list = []) -> None:
		self.light_definition = [job_extensions.default_light_definition] if (light_definition is None) else light_definition.copy()
		self.lighting_model = [job_extensions.default_lighting_model] if (lighting_model is None) else lighting_model.copy()
		self.lit_shader = [job_extensions.default_lit_shader] if (lit_shader is None) else lit_shader.copy()

class job_args:
	def __init__(self, directories: job_directories = job_directories(), extensions: job_extensions = job_extensions()) -> None:
		self.directories = directories
		self.extensions = extensions

	def merged_light_path(self) -> str:
		return os.path.join(self.directories.intermediate_dir, "LightDefinitions." + self.extensions.light_definition[0])

	def shader_data_path(self) -> str:
		return os.path.join(self.directories.output_dir, "ShaderData.json")

class source_info:
	def __init__(self, path: str, directory: str, is_dirty: bool) -> None:
		self.path = path
		self.directory = directory
		self.is_dirty = is_dirty

	def local_path(self) -> str:
		return os.path.join(os.path.basename(self.directory), os.path.relpath(self.path, self.directory))

	def __str__(self) -> str:
		return json.dumps({ 'path': self.path, 'directory': self.directory, 'dirty': self.is_dirty })

class builder:
	def __init__(self, arguments: job_args) -> None:
		self.__arguments = arguments
		self.__shader_data = jimara_shader_data.jimara_shader_data()
		self.__source_dependencies = jimara_source_dependencies.source_info(
			self.__arguments.directories.src_dirs + self.__arguments.directories.include_dirs,
			os.path.join(self.__arguments.directories.intermediate_dir, "shader_src_dependencies.json"))
		pass

	def build_shaders(self) -> None:
		self.__shader_data = jimara_shader_data.jimara_shader_data()
		self.__merge_light_definitions()
		self.__generate_lit_shaders()
		# __TODO__: For each combination of lit shaders and lighting models, generate compilation task
		# __TODO__: Filter out which shader & lighting models need recompiling and which ones are good to go
		# __TODO__: Find all non-lighting-model shaders that need recompiling
		# __TODO__: Compile all shaders that need (re)compiling in separate processes for maximal CPU utilization
		jimara_file_tools.update_text_file(self.__arguments.shader_data_path(), self.__shader_data.__str__())
		self.__source_dependencies.update_cache()
		exit(ERROR_NOT_YET_IMPLEMENTED)

	def __merge_light_definitions(self) -> None:
		light_definitions = []
		for directory in self.__arguments.directories.src_dirs: 
			light_definitions += jimara_file_tools.find_by_extension(directory, self.__arguments.extensions.light_definition)
		merged_light_path = self.__arguments.merged_light_path()
		if os.path.isfile(merged_light_path):
			should_rebuild = False
			for definition in light_definitions:
				if self.__source_dependencies.source_dirty(definition):
					should_rebuild = True
		else:
			should_rebuild = True
		if should_rebuild:
			merged_lights, buffer_elem_size = jimara_merge_light_shaders.merge_light_shaders(light_definitions)
			jimara_file_tools.update_text_file(merged_light_path, merged_lights)
			print("jimara_build_shaders.builder.__merge_light_definitions -> " + merged_light_path)
			for light_definition in jimara_file_tools.strip_file_extensions(jimara_file_tools.get_file_names(light_definitions)):
				self.__shader_data.add_light_type(light_definition, buffer_elem_size)
			if not self.__source_dependencies.source_dirty(merged_light_path):
				print("jimara_build_shaders.builder.__merge_light_definitions - Internal error: Generated light type header ('", merged_light_path, "') not recognized as dirty!")
				exit(ERROR_LIGHT_PATH_EXPECTED_DIRTY)

	def __generate_lit_shaders(self) -> list:
		def gather_sources(self: builder, extensions: list) -> list:
			sources = []
			for directory in self.__arguments.directories.src_dirs:
				for source in jimara_file_tools.find_by_extension(directory, extensions):
					sources.append(source_info(source, directory, self.__source_dependencies.source_dirty(source)))
			return sources
		lighting_models = gather_sources(self, self.__arguments.extensions.lighting_model)
		lit_shaders = gather_sources(self, self.__arguments.extensions.lit_shader)
		recompile_all = self.__source_dependencies.source_dirty(self.__arguments.merged_light_path())
		rv = []
		for i in range(len(lighting_models)):
			model = lighting_models[i]
			model_dir = self.__shader_data.get_lighting_model_directory(model.local_path())
			intermediate_dir = os.path.join(self.__arguments.directories.intermediate_dir, model_dir)
			output_dir = os.path.join(self.__arguments.directories.output_dir, model_dir)
			for j in range(len(lit_shaders)):
				shader = lit_shaders[j]
				shader_path = shader.local_path()
				intermediate_file = 
				# __TODO__: Get generated SPIR-V path and check if it exists before we proceed
		exit(ERROR_NOT_YET_IMPLEMENTED)


def main() -> None:
	def add_source_dir(args: job_args, value: str) -> None:
		args.directories.src_dirs.append(value)
	
	def add_include_dir(args: job_args, value: str) -> None:
		args.directories.include_dirs.append(value)
	
	def set_intermediate_dir(args: job_args, value: str) -> None:
		if args.directories.intermediate_dir is not None:
			print("jimara_build_shaders - Warning: intermediate directory provided more than once! (last one will be used)")
		args.directories.intermediate_dir = value
	
	def set_output_dir(args: job_args, value: str) -> None:
		if args.directories.output_dir is not None:
			print("jimara_build_shaders - Warning: output directory provided more than once! (last one will be used)")
		args.directories.output_dir = value

	def set_light_definition_ext(args: job_args, value: str) -> None:
		args.extensions.light_definition = value

	def set_lighting_model_ext(args: job_args, value: str) -> None:
		args.extensions.lighting_model = value

	def set_lit_shader_ext(args: job_args, value: str) -> None:
		args.extensions.lit_shader = value

	args = job_args()
	commands = {
		"--source": add_source_dir,
		"-s": add_source_dir,
		"--include": add_include_dir,
		"-i": add_include_dir,
		"--intermediate": set_intermediate_dir,
		"-id": set_intermediate_dir,
		"--output": set_output_dir,
		"-o": set_output_dir,
		"--light-definition-ext": set_light_definition_ext,
		"-jld": set_light_definition_ext,
		"--lighting-model-ext": set_lighting_model_ext,
		"-jlm": set_lighting_model_ext,
		"--lit-shader-ext": set_lit_shader_ext,
		"-jls": set_lit_shader_ext
	}

	def print_instructions():
		print(
			"Usage: \"python jimara_build_shaders.py <command0> values... <command1> values... <...> ...\" with possible commands:\n" +
			"    ?: Prints instructions;\n"
			"    --source, -s: Source directories (this is used by default if no command is provided);\n" +
			"    --include, -i: Include directories (sources from the directories listed after any of these flags will be included during compilation);\n" + 
			"    --intermediate, -id: Intermediate directory for the building process (Required; largely unimportant after compilation, but stores data for incremental builds);\n" +
			"    --output, -o: Output directory (Required; Compiled shaders and light and shader data json will be stored here);\n" + 
			"    --light-definition-ext, -jld: Specifies light definition extension ('" + job_extensions.default_light_definition + "' by default; more than one is allowed);\n" + 
			"    --lighting-model-ext, -jlm: Specifies lighting model extension ('" + job_extensions.default_lighting_model + "' by default; more than one is allowed);\n" + 
			"    --lit-shader-ext, -jls: Specifies lit shader extension ('" + job_extensions.default_lit_shader + "' by default; more than one is allowed)\n")
	
	i = 1
	last_command = "-s"
	while i < len(sys.argv):
		arg = sys.argv[i]
		i += 1
		if arg == '?':
			print_instructions()
		elif arg in commands:
			last_command = arg
		else:
			commands[last_command](args, arg)
	
	if args.directories.intermediate_dir is None:
		print("jimara_build_shaders - Intermediate directory not provided!")
		exit(ERROR_MISSING_ARGUMENTS)
	elif args.directories.output_dir is None:
		print("jimara_build_shaders - Output directory not provided!")
		exit(ERROR_MISSING_ARGUMENTS)
	
	if len(args.extensions.light_definition) <= 0:
		args.extensions.light_definition.append(job_extensions.default_light_definition)
	if len(args.extensions.lighting_model) <= 0:
		args.extensions.lighting_model.append(job_extensions.default_lighting_model)
	if len(args.extensions.lit_shader) <= 0:
		args.extensions.lit_shader.append(job_extensions.default_lit_shader)

	builder(args).build_shaders()

if __name__ == "__main__":
	main()
