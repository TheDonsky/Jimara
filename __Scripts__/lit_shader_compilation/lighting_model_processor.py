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


# Available Shader stages:
JM_ComputeShader         = shader_stage('JM_ComputeShader',         (1 << 0), 'comp')

JM_VertexShader          = shader_stage('JM_VertexShader',          (1 << 1), 'vert')
JM_FragmentShader        = shader_stage('JM_FragmentShader',        (1 << 2), 'frag')

JM_RayGenShader          = shader_stage('JM_RayGenShader',          (1 << 3), 'rgen')
JM_RayMissShader         = shader_stage('JM_RayMissShader',         (1 << 4), 'rmiss')
JM_RayAnyHitShader       = shader_stage('JM_RayAnyHitShader',       (1 << 5), 'rahit')
JM_RayClosestHitShader   = shader_stage('JM_RayClosestHitShader',   (1 << 6), 'rchit')
JM_RayIntersectionShader = shader_stage('JM_RayIntersectionShader', (1 << 7), 'rint')
JM_CallableShader        = shader_stage('JM_CallableShader',        (1 << 8), 'rcall')

shader_stages: dict[str, shader_stage] = {
	JM_ComputeShader.name         : JM_ComputeShader,

	JM_VertexShader.name          : JM_VertexShader,
	JM_FragmentShader.name        : JM_FragmentShader,

	JM_RayGenShader.name          : JM_RayGenShader,
	JM_RayMissShader.name         : JM_RayMissShader,
	JM_RayAnyHitShader.name       : JM_RayAnyHitShader,
	JM_RayClosestHitShader.name   : JM_RayClosestHitShader,
	JM_RayIntersectionShader.name : JM_RayIntersectionShader,
	JM_CallableShader.name        : JM_CallableShader
}

def generate_shader_stage_macro_definitions() -> str:
	rv = ''
	for stage_def in shader_stages.values():
		rv += "#define " + stage_def.name + " " + str(stage_def.value) + '\n'
	return rv


# Available LM-Stage Flags:
JM_NoLitShader                 = (1 << 16)
JM_VertexDisplacementSupported = (1 << 17)
JM_Rng                         = (1 << 18)
JM_ShadowQuerySupported        = (1 << 19)
JM_ShadowQueryTransparent      = (1 << 20)
JM_OffscreenFragmentQueries    = (1 << 21)

shader_stage_flags: dict[str, int] = {
	'JM_NoLitShader': JM_NoLitShader,
	'JM_VertexDisplacementSupported': JM_VertexDisplacementSupported,
	'JM_Rng':                         JM_Rng,
	'JM_ShadowQuerySupported':        JM_ShadowQuerySupported,
	'JM_ShadowQueryTransparent':      JM_ShadowQueryTransparent,
	'JM_OffscreenFragmentQueries':    JM_OffscreenFragmentQueries
}

def generate_shader_stage_flag_definitions() -> str:
	rv = ''
	for flag_name in shader_stage_flags:
		rv += "#define " + flag_name + " " + str(shader_stage_flags[flag_name]) + '\n'
	rv += '#define JM_JM_LMStageHasFlag(jm_flag) ((JM_LMStageFlags & jm_flag) != 0)\n'
	return rv


# LM-Stage:
class lighting_model_stage:
	def __init__(self, name: str) -> None:
		self.name = name
		self.__stages: dict[str, shader_stage] = {}
		self.__flags: int = 0

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
			self.add_flag(stage.value)

	def remove_stage(self, stage: shader_stage) -> None:
		if stage.name in self.__stages:
			assert(self.__stages[stage.name] == stage)
			del self.__stages[stage.name]
			self.remove_flag(stage.value)

	def add_stage_by_name(self, stage_name: str) -> None:
		assert(stage_name in shader_stages)
		self.add_stage(shader_stages[stage_name])

	def remove_stage_by_name(self, stage_name: str) -> None:
		if stage_name in self.__stages:
			self.remove_stage(self.__stages[stage_name])

	def flags(self) -> int:
		return self.__flags

	def has_flag(self, flag: int) -> bool:
		return (self.__flags & flag) != 0

	def add_flag(self, flag: int) -> None:
		self.__flags |= flag

	def remove_flag(self, flag: int) -> None:
		self.__flags &= ~flag

	def add_flag_by_name(self, flag_name: str) -> None:
		if flag_name in shader_stage_flags:
			self.add_flag(shader_stage_flags[flag_name])
	
	def remove_flag_by_name(self, flag_name: str) -> None:
		if flag_name in shader_stage_flags:
			self.remove_flag(shader_stage_flags[flag_name])

	def __str__(self) -> str:
		result = '(Name: "' + self.name + '"; Flags: ' + str(self.flags()) + ' ['
		for flag in shader_stage_flags:
			if not self.has_flag(shader_stage_flags[flag]):
				continue
			if result[-1] != '[':
				result += ', '
			result += flag
		result += ']; Stages: ['
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
			elif token.token.token in shader_stages:
				stage.add_stage_by_name(token.token.token)
			elif token.token.token in shader_stage_flags:
				stage.add_flag_by_name(token.token.token)
			else:
				raise Exception(JM_LightingModelStage_pragma_name + ' - "' + token.token.token + '" Not a valid shader stage or flag! ' + str(args))
	preprocessor.pragma_handlers[JM_LightingModelStage_pragma_name] = process_JM_LightingModelStage_pragma

	preprocessor.include_file(jlm_path)
	return result


if __name__ == "__main__":
	cache = source_cache([os.path.realpath(os.path.dirname(os.path.realpath(__file__)) + "/../../__Source__")])
	model_data = parse_lighting_model(cache, "Jimara/Environment/Rendering/LightingModels/JLM_Template.jlm.sample")
	print(model_data)
