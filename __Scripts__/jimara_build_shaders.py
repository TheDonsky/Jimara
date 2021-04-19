import jimara_file_tools, jimara_merge_light_shaders, jimara_generate_lit_shaders, sys, os

instructions = (
	"Usage: python jimara_build_shaders.py src_dirs generated_gl_dir compiled_gl_dir light_header <light_glsl> <light_exts> <model_exts> <lit_shader_exts> <generated_gl_ext>\n" + 
	"    src_dirs           - Source directories, separated by '|' symbol, to search all source files in; \n" +
	"    generated_gl_dir   - Output directory for generated glsl sources (nested folder heirarchy will be generated automatically based on shader and lighting model names); \n" +
	"    compiled_spirv_dir - Output directory for compiled spir-v binaries (nested folder heirarchy will be generated automatically based on shader and lighting model names); \n" + 
	"    light_header       - Header file path for storing light type identifier mappings; \n" + 
	"    light_glsl         - GLSL file path for storing the merged lighting code (defaults to generated_gl_dir/Jimara_MergedLights.jld); \n" + 
	"    light_exts         - Light shader extensions, separated by '|' symbol, to search light types with (defaults to 'jld'<Jimara Light Definition>); \n" +
	"    model_exts         - Lighting model source extensions, separated by '|' symbol, to search lighting models with (defaults to 'jlm'<Jimara Lighting Model>); \n" +
	"    lit_shader_exts    - Lit shader extensions, separated by '|' symbol to search lit shaders with (defaults to 'jls'<Jimara Lit Shader>); \n" +
	"    generated_gl_ext   - Extension for generated glsl source files (defaults to 'glsl'; valid: 'glsl/vert/frag');")

class job_arguments:
	def __init__(
		self, src_dirs, generated_gl_dir, compiled_spirv_dir, light_header, 
		light_glsl = None, light_exts = None, model_exts = None, lit_shader_exts = None, generated_gl_ext = None):
		def get_uniques(elements, default):
			return default if (elements is None) else [i for i in set(elements)]
		self.src_dirs = get_uniques(src_dirs, None)
		self.generated_gl_dir = generated_gl_dir
		self.compiled_spirv_dir = compiled_spirv_dir
		self.light_header = light_header
		self.light_glsl = light_glsl if ((light_glsl is not None) or (generated_gl_dir is None)) else os.path.join(generated_gl_dir, "Jimara_MergedLights.jld")
		self.light_exts = get_uniques(light_exts, ['jld'])
		self.model_exts = get_uniques(model_exts, ['jlm'])
		self.lit_shader_exts = get_uniques(lit_shader_exts, ['jls'])
		self.generated_gl_ext = generated_gl_ext if (generated_gl_ext is not None) else 'glsl'

	def incomplete(self):
		return (
			(self.src_dirs is None) or
			(self.generated_gl_dir is None) or
			(self.compiled_spirv_dir is None) or
			(self.light_header is None) or
			(self.light_glsl is None) or
			(self.light_exts is None) or
			(self.model_exts is None) or
			(self.lit_shader_exts is None) or
			(self.generated_gl_ext is None))

	def __str__(self):
		return (
			"jimara_build_shaders - JOB ARGUMENTS:\n" +
			"    src_dirs - " + repr(self.src_dirs) + ";\n" +
			"    generated_gl_dir   - " + repr(self.generated_gl_dir) + ";\n" +
			"    compiled_spirv_dir - " + repr(self.compiled_spirv_dir) + ";\n" +
			"    light_header       - " + repr(self.light_header) + ";\n" +
			"    light_glsl         - " + repr(self.light_glsl) + ";\n" +
			"    light_exts         - " + str(self.light_exts) + ";\n" +
			"    model_exts         - " + str(self.model_exts) + ";\n" +
			"    lit_shader_exts    - " + str(self.lit_shader_exts) + ";\n" +
			"    generated_gl_ext   - " + repr(self.generated_gl_ext) + ";\n")

def make_job_arguments(args = sys.argv[1:]):
	return job_arguments(
		[] if (len(args) <= 0) else args[0].split('|'),
		None if (len(args) <= 1) else args[1],
		None if (len(args) <= 2) else args[2],
		None if (len(args) <= 3) else args[3],
		None if (len(args) <= 4) else args[4],
		None if (len(args) <= 5) else args[5].split('|'),
		None if (len(args) <= 6) else args[6].split('|'),
		None if (len(args) <= 7) else args[7].split('|'),
		None if (len(args) <= 8) else args[8])

def merge_light_shaders(job_arguments):
	light_definitions = []
	for src_dir in job_arguments.src_dirs:
		light_definitions += jimara_file_tools.find_by_extension(src_dir, job_arguments.light_exts)
	def write_to_file(filename, data):
		dirname = os.path.dirname(filename)
		if (len(dirname) > 0) and (not os.path.isdir(dirname)):
			os.makedirs(dirname)
		with open(filename, "w") as file:
			file.write(data)
	merged_lights, buffer_elem_size = jimara_merge_light_shaders.merge_light_shaders(light_definitions)
	write_to_file(job_arguments.light_glsl, merged_lights)
	write_to_file(
		job_arguments.light_header, 
		jimara_merge_light_shaders.generate_engine_type_indices(
			light_definitions,
			jimara_file_tools.strip_file_extension(os.path.basename(job_arguments.light_header)),
			buffer_elem_size))

def generate_lit_shaders(job_arguments):
	generated_shaders = []
	for model_dir in job_arguments.src_dirs:
		for shader_dir in job_arguments.src_dirs:
			generate_job_args = jimara_generate_lit_shaders.job_arguments(
				job_arguments.light_glsl, 
				job_arguments.generated_gl_dir, model_dir, shader_dir,
				job_arguments.model_exts, job_arguments.lit_shader_exts, job_arguments.generated_gl_ext)
			print(generate_job_args)
			generate_job = jimara_generate_lit_shaders.job_description(generate_job_args)
			print(generate_job)
			jimara_generate_lit_shaders.execute_job(generate_job)
			for task in generate_job.tasks:
				generated_shaders.append(task.output)
	return generated_shaders

def compile_lit_shaders(shader_sources, gl_base_dir, out_dir, include_dirs):
	for shader in shader_sources:
		relpath = os.path.relpath(shader, gl_base_dir)
		name, ext = os.path.splitext(relpath)
		base_out_path = os.path.join(out_dir, name)
		base_out_dir = os.path.dirname(base_out_path)
		if (len(base_out_dir) > 0) and (not os.path.isdir(base_out_dir)):
			os.makedirs(base_out_dir)
		def compile(definitions, stage, output):
			command = "glslc -std=450 -fshader-stage=" + stage + " \"" + shader + "\" -o \"" + output + "\""
			for definition in definitions:
				command += " -D" + definition
			for include_dir in include_dirs:
				command += " -I\"" + include_dir + "\""
			print(command)
			error = os.system(command)
			if (error != 0):
				print(error)
				exit(error)
		if (ext == '.glsl') or (ext == '.vert'):
			compile(['JIMARA_VERTEX_SHADER'], 'vert', base_out_path + ".vert.spv")
		if (ext == '.glsl') or (ext == '.frag'):
			compile(['JIMARA_FRAGMENT_SHADER'], 'frag', base_out_path + ".frag.spv")


if __name__ == "__main__":
	job_arguments = make_job_arguments()
	if job_arguments.incomplete():
		print(instructions)
		exit()
	print(job_arguments)
	merge_light_shaders(job_arguments)
	generated_shaders = generate_lit_shaders(job_arguments)
	compile_lit_shaders(generated_shaders, job_arguments.generated_gl_dir, job_arguments.compiled_spirv_dir, job_arguments.src_dirs)
			