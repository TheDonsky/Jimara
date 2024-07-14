import sys

single_symbol_tokens = [
	'-', '+', '*', '/', '%',
	'!', '=', '~', '^', '|', '&', 
	'?', '.', ',', ';',
	'[', ']', '{', '}', '(', ')', '<', '>']

def tokenize_c_like(source : str) -> list:
	class tokenizer:
		def __init__(self) -> None:
			self.words = []
			self.word = ""
		
		def push_word(self) -> None:
			if len(self.word) > 0:
				self.words.append(self.word)
				self.word = ""

		def tokenize_source(self, source: str):
			old_words = self.words
			self.words = []
			i = 0
			count = len(source)
			while i < count:
				symbol = source[i]
				if symbol.isspace():
					self.push_word()
				elif (symbol == "'") or (symbol == '"'):
					self.push_word()
					self.word = symbol
					i += 1
					while i < count:
						character = source[i]
						self.word += character
						i += 1
						if character == '\\':
							if i < count:
								self.word += source[i]
								i += 1
							else:
								self.word += character
						elif character == symbol:
							break
					if self.word[-1] != symbol:
						self.word += symbol
					self.push_word()
				elif (symbol == '/') and (i < (count - 1)) and (source[i + 1] == '/'):
					self.push_word()
					while i < count and source[i] != '\n':
						i += 1
				elif (symbol == '/') and (i < (count - 1)) and (source[i + 1] == '*'):
					self.push_word()
					i += 2
					while i < count:
						should_break = (i < (count - 2)) and (source[i] == '*') and (source[i + 1] == '/')
						i += 1
						if should_break:
							break
				elif symbol in single_symbol_tokens:
					self.push_word()
					self.words.append(symbol)
				else:
					self.word += symbol
				i += 1
			self.push_word()

			word_count = len(self.words)
			i = 0
			while i < word_count:
				word = self.words[i]
				old_words.append(word)
				i += 1
				if (word == "#include") and (i < word_count) and (self.words[i] == '<'):
					name = ""
					while i < word_count:
						part = self.words[i]
						i += 1
						name += part
						if part == '>':
							break
					if name[-1] != '>':
						name += '>'
					old_words.append(name)

			self.words = old_words
			return self
	return tokenizer().tokenize_source(source).words


if __name__ == "__main__":
	if len(sys.argv) < 2:
		print("Usage: python jimara_tokenize_source.py <File0> <File1> ...")
	for filename in sys.argv[1:]:
		with open(filename) as file:
			text = file.read()
		print(filename + ":")
		for i, word in enumerate(tokenize_c_like(text)):
			print(i.__str__() + ": " + word)
		print('\n', '\n', '\n')
