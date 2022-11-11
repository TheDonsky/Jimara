import os, sys, json
import jimara_file_tools
import jimara_shader_data
import jimara_source_dependencies
import jimara_merge_light_shaders
import jimara_generate_lit_shaders

ERROR_ANY = 1
ERROR_NOT_YET_IMPLEMENTED = 2
ERROR_MISSING_ARGUMENTS = 3
ERROR_LIGHT_PATH_EXPECTED_DIRTY = 4

class job_directories:
	def __init__(self, src_dirs: list = [], include_dirs: list = [], intermediate_dir: str = None, output_dir: str = None) -> None:
		self.src_dirs = src_dirs.copy()
		self.include_dirs = include_dirs.copy() + self.src_dirs
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
		basename = os.path.basename(self.directory.strip('/').strip('\\'))
		return os.path.join(basename, os.path.relpath(self.path, self.directory))

	def __str__(self) -> str:
		return json.dumps({ 'path': self.path, 'directory': self.directory, 'dirty': self.is_dirty })

class compilation_task:
	def __init__(self, source_path: str, output_dir: str, include_dirs: list, is_lit_shader: bool) -> None:
		self.source_path = source_path
		self.output_dir = output_dir
		self.include_dirs = include_dirs
		self.is_lit_shader = is_lit_shader

	class output_file:
		def __init__(self, path: str, stage: str) -> None:
			self.path = path
			self.stage = stage

	def output_files(self) -> list:
		filename = os.path.basename(self.source_path)
		name, extension = os.path.splitext(filename)
		out_path = os.path.join(self.output_dir, name)
		if extension == '.glsl':
			return [
				compilation_task.output_file(out_path + ".vert.spv", 'vert'), 
				compilation_task.output_file(out_path + ".frag.spv", 'frag')
			]
		elif (extension == '.vert') or (extension == '.frag') or (extension == '.comp'):
			return [compilation_task.output_file(out_path + extension + '.spv', extension.strip('.'))]
		else:
			print("jimara_build_shaders.builder.__output_files - '" + self.source_path + "' shader extension unknown!")
			return []

	def execute(self) -> None:
		if not os.path.isdir(self.output_dir):
			os.makedirs(self.output_dir)

		includes = ""
		for include_dir in self.include_dirs:
			includes += " -I\"" + include_dir + "\""

		def compile(out_file: compilation_task.output_file, definitions: list):
			print(self.source_path + "\n    -> " + out_file.path)
			command = (
				"glslc -std=450 -fshader-stage=" + out_file.stage + 
				' "' + self.source_path + '" -o "' + out_file.path + "\"")
			for definition in definitions:
				command += " -D" + definition
			command += includes
			error = os.system(command)
			if error != 0:
				print("Error code: " + str(error) + "\nCommand: " + command)
				sys.exit(error)

		for out_file in self.output_files():
			if self.is_lit_shader:
				if out_file.stage == "vert":
					definitions = ['JIMARA_VERTEX_SHADER']
				elif out_file.stage == "frag":
					definitions = ['JIMARA_FRAGMENT_SHADER']
			else:
				definitions = []
			compile(out_file, definitions)


class builder:
	def __init__(self, arguments: job_args) -> None:
		self.__arguments = arguments
		self.__shader_data = jimara_shader_data.jimara_shader_data()
		self.__source_dependencies = jimara_source_dependencies.source_info(
			self.__arguments.directories.src_dirs + self.__arguments.directories.include_dirs,
			os.path.join(self.__arguments.directories.intermediate_dir, "shader_src_dependencies.json"))

	def build_shaders(self) -> None:
		self.__shader_data = jimara_shader_data.jimara_shader_data()
		self.__merge_light_definitions()
		compilation_tasks = (
			self.__generate_lit_shaders() + 
			self.__generate_general_shader_compilation_tasks())
		for task in compilation_tasks:
			task.execute()
		jimara_file_tools.update_text_file(self.__arguments.shader_data_path(), self.__shader_data.__str__())
		self.__source_dependencies.update_cache()

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
		merged_lights, buffer_elem_size = jimara_merge_light_shaders.merge_light_shaders(light_definitions)
		for light_definition in jimara_file_tools.strip_file_extensions(jimara_file_tools.get_file_names(light_definitions)):
				self.__shader_data.add_light_type(light_definition, buffer_elem_size)
		if should_rebuild:
			dirname = os.path.dirname(merged_light_path)
			if (len(dirname) > 0) and (not os.path.isdir(dirname)):
				os.makedirs(dirname)
			with open(merged_light_path, 'w') as merged_light_file:
				merged_light_file.write(merged_lights)
			print("jimara_build_shaders.builder.__merge_light_definitions\n    -> " + merged_light_path)
			if not self.__source_dependencies.source_dirty(merged_light_path):
				print("jimara_build_shaders.builder.__merge_light_definitions - Internal error: Generated light type header ('", merged_light_path, "') not recognized as dirty!")
				sys.exit(ERROR_LIGHT_PATH_EXPECTED_DIRTY)

	def __gather_sources(self, extensions: list) -> list:
		sources = []
		for directory in self.__arguments.directories.src_dirs:
			for source in jimara_file_tools.find_by_extension(directory, extensions):
				sources.append(source_info(source, directory, self.__source_dependencies.source_dirty(source)))
		return sources

	def __generate_lit_shaders(self) -> list:
		lighting_models = self.__gather_sources(self.__arguments.extensions.lighting_model)
		lit_shaders = self.__gather_sources(self.__arguments.extensions.lit_shader)
		light_header_path = self.__arguments.merged_light_path()
		recompile_all = self.__source_dependencies.source_dirty(light_header_path)
		rv = []
		source_cache = {}
		for i in range(len(lighting_models)):
			model: source_info = lighting_models[i]
			model_path = model.local_path()
			model_dir = self.__shader_data.get_lighting_model_directory(model_path)
			intermediate_dir = os.path.join(self.__arguments.directories.intermediate_dir, model_dir)
			output_dir = os.path.join(self.__arguments.directories.output_dir, model_dir)
			for j in range(len(lit_shaders)):
				shader: source_info = lit_shaders[j]
				shader_path = shader.local_path()
				shader_intermediate_name = os.path.splitext(shader_path)[0] + '.glsl'
				intermediate_file = os.path.join(intermediate_dir, shader_intermediate_name)
				task = compilation_task(
					intermediate_file, 
					os.path.dirname(os.path.join(output_dir, shader_intermediate_name)),
					self.__arguments.directories.include_dirs + [os.path.dirname(shader.path)], True)
				if recompile_all or model.is_dirty or shader.is_dirty:
					recompile = True
				else:
					recompile = False
					for output_file in task.output_files():
						if not os.path.isfile(output_file.path):
							recompile = True
				if recompile:
					print(model_path + " + " + shader_path + "\n    -> '" + intermediate_file + "'")
					jimara_generate_lit_shaders.generate_shader(light_header_path, model.path, shader.path, intermediate_file, source_cache)
					rv.append(task)
		return rv

	def __generate_general_shader_compilation_tasks(self) -> list:
		output_dir = os.path.join(self.__arguments.directories.output_dir, self.__shader_data.get_general_shader_directory())
		shaders = self.__gather_sources(['.glsl', '.frag', '.vert', '.comp'])
		rv = []
		for source in shaders:
			task = compilation_task(
				source.path, 
				os.path.dirname(os.path.join(output_dir, source.local_path())),
				self.__arguments.directories.include_dirs, False)
			if not self.__source_dependencies.source_dirty(source.path):
				output_exists = True
				for output_file in task.output_files():
					if not os.path.isfile(output_file.path):
						output_exists = False
				if output_exists:
					continue
			rv.append(task)
		return rv


def main() -> None:
	def add_source_dir(args: job_args, value: str) -> None:
		args.directories.src_dirs.append(value)
		args.directories.include_dirs.append(value)
	
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
		sys.exit(ERROR_MISSING_ARGUMENTS)
	elif args.directories.output_dir is None:
		print("jimara_build_shaders - Output directory not provided!")
		sys.exit(ERROR_MISSING_ARGUMENTS)
	
	if len(args.extensions.light_definition) <= 0:
		args.extensions.light_definition.append(job_extensions.default_light_definition)
	if len(args.extensions.lighting_model) <= 0:
		args.extensions.lighting_model.append(job_extensions.default_lighting_model)
	if len(args.extensions.lit_shader) <= 0:
		args.extensions.lit_shader.append(job_extensions.default_lit_shader)

	builder(args).build_shaders()

if __name__ == "__main__":
	main()
