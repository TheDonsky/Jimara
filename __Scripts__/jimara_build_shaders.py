import os, sys, json, multiprocessing
import jimara_file_tools
import jimara_shader_data
import jimara_source_dependencies
from code_analysis import source_cache
from lit_shader_compilation import light_definition_processor, lit_shader_processor, lighting_model_processor, lit_shader_compilation_utils
from typing import Protocol
from collections.abc import Iterable


ERROR_ANY = 1
ERROR_NOT_YET_IMPLEMENTED = 2
ERROR_MISSING_ARGUMENTS = 3
ERROR_LIGHT_PATH_EXPECTED_DIRTY = 4
ERROR_INTERNAL = 5

class glsl_extensions:
	generic = '.glsl'
	
	compute_shader = '.comp'
	vertex_shader = '.vert'
	fragment_shader = '.frag'
	
	ray_generation = '.rgen'
	ray_miss = '.rmiss'
	ray_any_hit = '.rahit'
	ray_closest_hit = '.rchit'
	ray_intersection = '.rint'
	callable = '.rcall'

	graphics_pipeline_stages = [vertex_shader, fragment_shader]
	rt_pipeline_stages = [ray_generation, ray_miss, ray_any_hit, ray_closest_hit, ray_intersection, callable]

	concrete_stages = [compute_shader] + graphics_pipeline_stages + rt_pipeline_stages
	all_extensions = [generic] + concrete_stages

class job_directories:
	def __init__(self, src_dirs: list[str] = [], include_dirs: list[str] = [], intermediate_dir: str = None, output_dir: str = None) -> None:
		self.src_dirs = src_dirs.copy()
		self.include_dirs = include_dirs.copy() + self.src_dirs
		self.intermediate_dir = intermediate_dir
		self.output_dir = output_dir

class job_extensions:
	default_light_definition = 'jld'
	default_lighting_model = 'jlm'
	default_lit_shader = 'jls'

	def __init__(self, light_definition: list[str] = [], lighting_model: list[str] = [], lit_shader: list[str] = []) -> None:
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

	def include_path(self) -> str:
		return os.path.relpath(self.path, self.directory)

	def local_path(self) -> str:
		basename = os.path.basename(self.directory.strip('/').strip('\\'))
		return os.path.join(basename, self.include_path())

	def __str__(self) -> str:
		return json.dumps({ 'path': self.path, 'directory': self.directory, 'dirty': self.is_dirty })


class compilation_task(Protocol):
	def __init__(self) -> None:
		super().__init__()

	def output_files(self) -> Iterable[str]:
		pass

	def execute(self) -> int:
		raise NotImplementedError()


class direct_compilation_task(compilation_task):
	def __init__(self, src_path: str, spv_path: str, include_dirs: list[str], definitions: list[str], stage: str) -> None:
		super().__init__()
		self.src_path = src_path
		self.spv_path = spv_path
		self.include_dirs = include_dirs
		self.definitions = definitions
		self.stage = stage

	def output_files(self) -> Iterable[str]:
		return [self.spv_path]

	def execute(self) -> int:
		jimara_file_tools.create_dir_if_does_not_exist(os.path.dirname(self.spv_path))
		command = (
			"glslc --target-env=vulkan1.2 -fshader-stage=" + self.stage + 
			' "' + self.src_path + '" -o "' + self.spv_path + "\"")
		for definition in self.definitions:
			command += " -D" + definition
		for include_dir in self.include_dirs:
			command += " -I\"" + include_dir + "\""
		error = os.system(command)
		if error != 0:
			print("Error code: " + str(error) + "\nCommand: " + command)
		else:
			print(self.src_path + "\n    -> " + self.spv_path)
		return error


class lit_shader_compilation_task(compilation_task):
	def __init__(self, 
			  lit_shader: lit_shader_processor.lit_shader_data, 
			  lighting_model: lighting_model_processor.lighting_model_data,
			  include_dirs: list[str],
			  light_definition_path: str,
			  intermediate_file: str,
			  spirv_dir: str) -> None:
		super().__init__()
		self.lit_shader = lit_shader
		self.lighting_model = lighting_model
		self.include_dirs = include_dirs
		self.light_definition_path = os.path.realpath(light_definition_path)
		self.intermediate_file = os.path.realpath(intermediate_file)
		self.spirv_dir = os.path.realpath(spirv_dir)

	def __gen_compilation_tasks(self) -> Iterable[direct_compilation_task]:
		filename = os.path.basename(self.intermediate_file)
		name, _ = os.path.splitext(filename)
		res: list[direct_compilation_task] = []
		for lm_stage in self.lighting_model.stages():
			if lm_stage.has_flag(lighting_model_processor.JM_NoLitShader) != (self.lit_shader is None):
				continue
			for gl_stage in lm_stage.stages():
				macro_definitions = [(s.name + ("=1" if (s is lm_stage) else "=0")) for s in self.lighting_model.stages()]
				macro_definitions.append('JM_ShaderStage=' + str(gl_stage.value))
				macro_definitions.append('JM_LMStageFlags=' + str(lm_stage.flags()))
				res.append(direct_compilation_task(
					src_path=self.intermediate_file, 
					spv_path=os.path.join(self.spirv_dir, name) + '.' + lm_stage.name + '.' + gl_stage.glsl_stage + '.spv',
					include_dirs=self.include_dirs,
					definitions = macro_definitions,
					stage=gl_stage.glsl_stage))
		return res

	def output_files(self) -> Iterable[str]:
		return [self.intermediate_file] + [task.spv_path for task in self.__gen_compilation_tasks()]

	def execute(self) -> int:
		glsl_source = lit_shader_compilation_utils.generate_glsl_source(
			lit_shader=self.lit_shader, 
			lighting_model=self.lighting_model,
			light_definition_relative_path=os.path.relpath(self.light_definition_path, os.path.dirname(self.intermediate_file)))
		jimara_file_tools.update_text_file(self.intermediate_file, glsl_source)
		for task in self.__gen_compilation_tasks():
			val = task.execute()
			if val != 0:
				return val
		return 0


def jimara_build_shaders_compile_batch_asynch(tasks: list[compilation_task], _):
	for task in tasks:
		try:
			error = task.execute()
		except NotImplementedError as e:
			print(e)
			error = ERROR_NOT_YET_IMPLEMENTED
		except Exception as e:
			print(e)
			error = ERROR_ANY
		except:
			print('Unknown error type!')
			error = ERROR_INTERNAL
		if error != 0:
			sys.exit(error)
	sys.stdout.flush()
	sys.exit(0)


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

		print("\n\n\n" + 
			"______________________________________________" +
			"\njimara_build_shaders.builder.build_shaders - Collecting compilation tasks....")
		compilation_tasks = (
			self.__generate_lit_shaders() + 
			self.__generate_general_shader_compilation_tasks())

		total_task_count = len(compilation_tasks)
		thread_count = max(multiprocessing.cpu_count(), 1)
		tasks_per_thread = int((total_task_count + thread_count - 1) / thread_count)

		processes = []
		task_ptr = 0
		while task_ptr < total_task_count:
			task_end = min(task_ptr + tasks_per_thread, total_task_count)
			subtasks = compilation_tasks[task_ptr:task_end]
			task_ptr = task_end
			process = multiprocessing.Process(
				target=jimara_build_shaders_compile_batch_asynch, 
				args=(subtasks, None))
			processes.append(process)
		print(
			"______________________________________________" +
			"\njimara_build_shaders.builder.build_shaders:" +
			"\n    Task count: " + total_task_count.__str__() +
			"\n    System Thread Count: " + thread_count.__str__() + 
			"\n    Tasks Per Thread: " + tasks_per_thread.__str__() + 
			"\n    Compilation processes: " + len(processes).__str__() + 
			"\n\n\n")

		if len(processes) > 0:
			print("jimara_build_shaders.builder.build_shaders - Starting tasks....")
			sys.stdout.flush()
			for task in processes:
				task.start()
			for task in processes:
				task.join()
			sys.stdout.flush()
			print("jimara_build_shaders.builder.build_shaders - Tasks joined....")
			print("Results: " + [task.exitcode for task in processes].__str__())
			for task in processes:
				if task.exitcode != 0:
					sys.exit(task.exitcode)

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
		merged_lights, buffer_elem_size = light_definition_processor.merge_light_shaders(light_definitions)
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

	def __gather_sources(self, extensions: list) -> list[source_info]:
		sources = []
		for directory in self.__arguments.directories.src_dirs:
			for source in jimara_file_tools.find_by_extension(directory, extensions):
				sources.append(source_info(source, directory, self.__source_dependencies.source_dirty(source)))
		return sources

	def __generate_lit_shaders(self) -> list[compilation_task]:
		src_cache = source_cache.source_cache(self.__arguments.directories.include_dirs)
		lighting_model_paths = self.__gather_sources(self.__arguments.extensions.lighting_model)
		lighting_models = [lighting_model_processor.parse_lighting_model(src_cache, model.include_path()) for model in lighting_model_paths]
		lit_shader_paths = self.__gather_sources(self.__arguments.extensions.lit_shader)
		lit_shaders = [lit_shader_processor.parse_lit_shader(src_cache, shader.include_path()) for shader in lit_shader_paths]
		for lit_shader in lit_shaders:
			self.__shader_data.add_lit_shader(lit_shader)

		light_header_path = self.__arguments.merged_light_path()
		recompile_all = self.__source_dependencies.source_dirty(light_header_path)
		rv = []
		for lighting_model_id in range(len(lighting_model_paths)):
			model: source_info = lighting_model_paths[lighting_model_id]
			model_path = model.local_path()
			model_dir = self.__shader_data.get_lighting_model_directory(model_path)
			intermediate_dir = os.path.join(self.__arguments.directories.intermediate_dir, model_dir)
			output_dir = os.path.join(self.__arguments.directories.output_dir, model_dir)
			if any([lm_stage.has_flag(lighting_model_processor.JM_NoLitShader) for lm_stage in lighting_models[lighting_model_id].stages()]):
				intermediate_file = os.path.join(intermediate_dir,  os.path.splitext(os.path.basename(model_path))[0] + glsl_extensions.generic)
				comp_task = lit_shader_compilation_task(
					lit_shader=None, 
					lighting_model=lighting_models[lighting_model_id], 
					include_dirs=self.__arguments.directories.include_dirs,
					light_definition_path=light_header_path,
					intermediate_file=intermediate_file,
					spirv_dir=output_dir)
				if recompile_all or model.is_dirty or any([(not os.path.isfile(output_file)) for output_file in comp_task.output_files()]):
					print(model_path + "\n    -> '" + intermediate_file + "'")
					rv.append(comp_task)
			if not any([(not lm_stage.has_flag(lighting_model_processor.JM_NoLitShader)) for lm_stage in lighting_models[lighting_model_id].stages()]):
				continue
			for lit_shader_id in range(len(lit_shader_paths)):
				shader: source_info = lit_shader_paths[lit_shader_id]
				shader_path = shader.local_path()
				shader_intermediate_name = os.path.splitext(shader_path)[0] + glsl_extensions.generic
				intermediate_file = os.path.join(intermediate_dir, shader_intermediate_name)
				spirv_directory = os.path.dirname(os.path.join(output_dir, shader_intermediate_name))
				comp_task = lit_shader_compilation_task(
					lit_shader=lit_shaders[lit_shader_id], 
					lighting_model=lighting_models[lighting_model_id], 
					include_dirs=self.__arguments.directories.include_dirs,
					light_definition_path=light_header_path,
					intermediate_file=intermediate_file,
					spirv_dir=spirv_directory)
				if recompile_all or model.is_dirty or shader.is_dirty:
					recompile = True
				else:
					recompile = False
					for output_file in comp_task.output_files():
						if not os.path.isfile(output_file):
							recompile = True
				if recompile:
					print(model_path + " + " + shader_path + "\n    -> '" + intermediate_file + "'")
					rv.append(comp_task)
		return rv

	def __generate_general_shader_compilation_tasks(self) -> list:
		output_dir = os.path.join(self.__arguments.directories.output_dir, self.__shader_data.get_general_shader_directory())
		shaders = self.__gather_sources(glsl_extensions.all_extensions)
		rv = []
		for source in shaders:
			abs_path = os.path.realpath(source.path)
			filename = os.path.basename(abs_path)
			name, extension = os.path.splitext(filename)
			out_path = os.path.join(os.path.dirname(os.path.join(output_dir, source.local_path())), name) + extension + '.spv'
			if self.__source_dependencies.source_dirty(source.path) or (not os.path.isfile(out_path)):
				rv.append(direct_compilation_task(
					abs_path, out_path, 
					self.__arguments.directories.include_dirs, [], 
					extension.strip('.')))
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
