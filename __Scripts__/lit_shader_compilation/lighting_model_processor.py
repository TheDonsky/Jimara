import sys, os
if __name__ == "__main__":
	sys.path.insert(0, os.path.realpath(os.path.dirname(os.path.realpath(__file__)) + "/../"))
from code_analysis.source_cache import source_cache
from code_analysis.preprocessor import preporocessor_state, source_line, source_string_repr
from code_analysis.syntax_tree_extractor import extract_syntax_tree_from_source_line
from collections.abc import Iterable


JM_LightingModelStage_pragma_name = 'JM_LightingModelStage'
JM_ShaderStage_macro_name = 'JM_ShaderStage'

class shader_stage:
	def __init__(self, name: str, value: int, glsl_stage: str) -> None:
		self.name = name
		self.value = value
		self.glsl_stage = glsl_stage

shader_stages: dict[str, shader_stage]
shader_stages = {
	'JM_ComputeShader'         : shader_stage('JM_ComputeShader', 0, 'comp'),

	'JM_VertexShader'          : shader_stage('JM_VertexShader', 1, 'vert'),
	'JM_FragmentShader'        : shader_stage('JM_FragmentShader', 2, 'frag'),

	'JM_RayGenShader'          : shader_stage('JM_RayGenShader', 3, 'rgen'),
	'JM_RayMissShader'         : shader_stage('JM_RayMissShader', 4, 'rmiss'),
	'JM_RayAnyHitShader'       : shader_stage('JM_RayAnyHitShader', 5, 'rahit'),
	'JM_RayClosestHitShader'   : shader_stage('JM_RayClosestHitShader', 6, 'rchit'),
	'JM_RayIntersectionShader' : shader_stage('JM_RayIntersectionShader', 7, 'rint'),
	'JM_CallableShader'        : shader_stage('JM_CallableShader', 8, 'rcall')
}

def generate_shader_stage_macro_definitions() -> str:
	rv = ''
	for stage_def in shader_stages.values():
		rv += "#define " + stage_def.name + " " + str(stage_def.value) + '\n'
	return rv


class lighting_model_stage:
	def __init__(self, name: str) -> None:
		self.name = name
		self.__stages: dict[str, shader_stage]
		self.__stages = {}

	def stages(self) -> Iterable[shader_stage]:
		return self.__stages.values()
	
	def has_stage(self, stage) -> bool:
		if isinstance(stage, shader_stage):
			return stage.name in self.__stages
		return stage in self.__stages
	
	def add_stage(self, stage: shader_stage) -> None:
		if stage.name in self.__stages:
			assert(self.__stages[stage.name] == stage)
		else:
			self.__stages[stage.name] = stage

	def remove_stage(self, stage: shader_stage) -> None:
		if stage.name in self.__stages:
			assert(self.__stages[stage.name] == stage)
			del self.__stages[stage.name]

	def add_stage_by_name(self, stage_name: str) -> None:
		assert(stage_name in shader_stages)
		self.__stages[stage_name] = shader_stages[stage_name]

	def remove_stage_by_name(self, stage_name: str) -> None:
		if stage_name in self.__stages:
			del self.__stages[stage_name]

	def __str__(self) -> str:
		result = '(Name: "' + self.name + '" Stages: ['
		i = 0
		for stage in self.stages():
			if i > 0:
				result += ', '
			result += stage.name
			i += 1
		result += '])'
		return result


class lighting_model_data:
	def __init__(self, path: str, jlm_path: str) -> None:
		self.path = path
		self.jlm_path = jlm_path
		self.__stages = dict[str, lighting_model_stage]
		self.__stages = {}

	def stage(self, stage_name: str) -> lighting_model_stage:
		if stage_name not in self.__stages:
			self.__stages[stage_name] = lighting_model_stage(stage_name)
		return self.__stages[stage_name]
	
	def remove_stage(self, stage) -> None:
		if isinstance(stage, lighting_model_stage):
			assert(stage.name in self.__stages)
			del self.__stages[stage.name]
		else:
			assert(stage in self.__stages)
			del self.__stages[stage]

	def stages(self) -> Iterable[lighting_model_stage]:
		return self.__stages.values()
	
	def __str__(self) -> str:
		result = '{\n'
		result += '    Source: ' + source_string_repr(self.path) + '\n'
		for stage in self.stages():
			result += '    ' + str(stage) + '\n'
		result += '}'
		return result
		
	
def parse_lighting_model(src_cache: source_cache, jlm_path: str) -> lighting_model_data:
	result = lighting_model_data(src_cache.get_local_path(jlm_path), jlm_path)
	preprocessor = preporocessor_state(src_cache)

	def process_JM_LightingModelStage_pragma(args: source_line):
		stage_tokens = extract_syntax_tree_from_source_line(args)
		stage = None
		for token in stage_tokens:
			if token.has_child_nodes() or token.end_bracket_token() is not None:
				continue
			elif len(token.token.token) <= 0:
				continue
			elif token.token.token == ',':
				pass
			elif token.token.token == ';':
				stage = None
			elif token.token.token[0].isnumeric():
				raise Exception(JM_LightingModelStage_pragma_name + ' - Unexpected token: "' + token.token.token + '"! ' + str(args))
			elif stage is None:
				stage = result.stage(token.token.token)
			elif token.token.token not in shader_stages:
				raise Exception(JM_LightingModelStage_pragma_name + ' - "' + token.token.token + '" Not a vlaid shader stage! ' + str(args))
			else:
				stage.add_stage_by_name(token.token.token)
	preprocessor.pragma_handlers[JM_LightingModelStage_pragma_name] = process_JM_LightingModelStage_pragma

	preprocessor.include_file(jlm_path)
	return result


if __name__ == "__main__":
	cache = source_cache([os.path.realpath(os.path.dirname(os.path.realpath(__file__)) + "/../../__Source__")])
	model_data = parse_lighting_model(cache, "Jimara/Environment/Rendering/LightingModels/JLM_Template.jlm.sample")
	print(model_data)
