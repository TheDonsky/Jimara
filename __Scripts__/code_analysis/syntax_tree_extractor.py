from source_cache import source_cache
from preprocessor import source_line, preporocessor_state
from jimara_tokenize_source import tokenize_c_like

BRACKET_PAIRS = {
	'(': ')',
	'{': '}',
	'[': ']'
}

class tok_and_line:
	''' Token and line tuple '''
	def __init__(self, token: str, line: source_line) -> None:
		self.token = token
		self.line = line


def tokenize_source_lines(lines: list[source_line]) -> list[tok_and_line]:
	'''
	Splits source lines into tok_and_line pair list

	Parameters
	----------
	lines : list[source_line]
		List of source lines

	Returns
	-------
	List of tok_and_line pairs
	'''
	words = []
	for src_line in lines:
		line_words = tokenize_c_like(src_line.processed_text)
		for line_word in line_words:
			words.append(tok_and_line(line_word, src_line))
	return words


class syntax_tree_node:
	''' A tok_and_line token and a list of sub-tokens '''

	def __init__(self, token: tok_and_line, child_nodes: list['syntax_tree_node']) -> None:
		self.token = token
		self.child_nodes = child_nodes

	def end_bracket_token(self):
		if self.token.token == '<':
			return '>'
		elif self.token.token in BRACKET_PAIRS:
			return BRACKET_PAIRS[self.token.token]
		return None

	def __str__(self) -> str:
		def to_str(node: syntax_tree_node, tab: str):
			result = tab + node.token.token
			if node.child_nodes:
				for child in node.child_nodes:
					result += '\n' + to_str(child, tab + '  ')
			end_token = node.end_bracket_token()
			if end_token is not None:
				result += '\n' + tab + end_token
			return result
		return to_str(self, '')


def extract_syntax_tree(tokens: list[tok_and_line], first_token_id: int = 0, closing_bracket: str = '') -> tuple[list[syntax_tree_node], int]:
	'''
	Given a list of tokens and argument list start token index, extracts a list of recursive syntax_tree_node objects

	Parameters:
	-----------
	tokens : list[tok_and_line]
		List of words/tokens
	first_token_id : int
		First token index within tokens
	closing_bracket : str
		Expected closing bracket to early-quit parsing in case we were extracting a subtree

	Returns:
	--------
	Tuple of syntax_tree_node list and the index of the first token that has not been parsed (or len(tokens), if the list end has been reached)
	'''
	nodes = []
	token_id = first_token_id
	while token_id < len(tokens):
		token = tokens[token_id]
		token_id += 1
		if (closing_bracket is not None) and (token.token == closing_bracket):
			return nodes, token_id
		elif token.token in BRACKET_PAIRS:
			subnodes, token_id = extract_syntax_tree(tokens, token_id, BRACKET_PAIRS[token.token])
			nodes.append(syntax_tree_node(token, subnodes))
		else:
			nodes.append(syntax_tree_node(token, []))
	return nodes, token_id



if __name__ == "__main__":
	cache = source_cache(["__Test__/include_dir_0", "__Test__/include_dir_1"])
	preprocessor = preporocessor_state(cache)
	preprocessor.include_file("subdir_a/file_a.h")
	tokens = tokenize_source_lines(preprocessor.line_list)
	syntax_tree = extract_syntax_tree(tokens)[0]
	for node in syntax_tree:
		print(node)
