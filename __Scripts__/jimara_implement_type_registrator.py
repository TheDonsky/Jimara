from posixpath import relpath
import jimara_file_tools, os, sys, re

instructions = (
	"Usage: python jimara_implement_type_registrator.py src_dirs reg_class reg_impl_header\n" +
	"    src_dirs         - Source directories, separated by '|' symbol, to search all header files with JIMARA_REGISTER_TYPE definitions;\n" +
	"    reg_class        - Name of the type registartor class defined with JIMARA_DEFINE_TYPE_REGISTRATION_CLASS command (full name with namespaces);\n" +
	"    reg_impl_header  - File path to output the implementation of reg_class too (note, that you have to include this implementation into your project for the thing to work).")


def remove_quotes_and_comments(text):
	clean_text = ""
	i = 0
	end = len(text)
	while i < end:
		symbol = text[i]
		if i < (end - 1):
			if symbol == '/':
				next = text[i + 1]
				if next == '/':
					while i < end and text[i] != '\n':
						i += 1
					continue
				elif next == "*":
					i += 2
					while i < end:
						if text[i] == '*':
							if i < (end - 1):
								if text[i + 1] == '/':
									break
							else:
								break
						i += 1
					continue
				pass
			elif symbol == '"':
				prev = symbol
				i += 1
				while i < end:
					cur = text[i]
					i += 1
					if cur == '\\' and prev == '\\':
						prev = ' '
						continue
					elif cur == '"' and prev != '\\':
						break
					prev = cur
				clean_text += '""'
				continue
		clean_text += symbol
		i += 1
	return clean_text

class registered_types:
	def __init__(self, src_file):
		self.src_file = src_file
		self.types = []
		with open(src_file, "r") as file:
			text = remove_quotes_and_comments(file.read())
		lines = text.splitlines()
		for line in lines:
			ln = ""
			for symbol in line:
				if symbol.isspace():
					ln += " "
				else:
					ln += symbol
			while True:
				old_ln = ln
				ln = ln.replace("  ", " ")
				if len(old_ln) == len(ln):
					break
			words = []
			for token in re.split(" |\(|\)", ln):
				if len(token) > 0:
					words.append(token)
			if len(words) <= 0:
				continue
			elif words[0] == "#define":
				continue
			else:
				last_word = len(words) - 1
				i = 0
				while i < last_word:
					if words[i] == "JIMARA_REGISTER_TYPE":
						self.types.append(words[i + 1])
						i += 1
					i += 1
		self.types = [t for t in set(self.types)]
		self.types.sort()

	def __str__(self):
		return "<file:" + repr(self.src_file) + "; types:" + str(self.types) + ">"

class job_arguments:
	def __init__(self, src_dirs = sys.argv[1], reg_class = sys.argv[2], reg_impl_header = sys.argv[3]):
		def find_all_headers():
			src_files = []
			for dir in src_dirs.split('|'):
				if len(dir) > 0:
					if os.path.isdir(dir):
						for file in jimara_file_tools.find_by_extension(dir, [".h", ".hpp", ".cuh"]):
							src_files.append(file)
					elif os.path.isfile(dir):
						src_files.append(dir)
			unique_files = [v for v in set(src_files)]
			unique_files.sort()
			registered = []
			for fl in unique_files:
				reg = registered_types(fl)
				if len(reg.types) > 0:
					registered.append(reg)
			return registered

		self.src_files = find_all_headers()
		self.reg_class = reg_class.split("::")
		self.reg_impl_header = reg_impl_header

	def __str__(self):
		text = "jimara_implement_type_registrator.job_arguments: {\n    src_files: ["
		for file in self.src_files:
			text += "\n        " + str(file)
		text +=	("]"
			"\n    reg_class: " + str(self.reg_class) +
			"\n    reg_impl_header: " + repr(self.reg_impl_header) + 
			"\n}")
		return text

def generate_source_text(job_args):
	source = ""
	for src_file in job_args.src_files:
		source += "#include \"" + str(os.path.relpath(src_file.src_file, os.path.dirname(job_args.reg_impl_header))).replace('\\', '/') + "\"\n"
	source += "#include <mutex>\n\n"

	inset = ""
	for i in range(len(job_args.reg_class) - 1):
		source += inset + "namespace " + job_args.reg_class[i] + " {\n"
		inset += '\t'
	
	reg_class_name = job_args.reg_class[len(job_args.reg_class) - 1]
	registration_lock_name = reg_class_name + "_Lock"
	registration_instance_name = reg_class_name + "_Instance"

	source += (
		inset + "namespace {\n" +
		inset + "\tstatic std::mutex " + registration_lock_name + ";\n" +
		inset + "\tstatic " + reg_class_name + "* volatile " + registration_instance_name + " = nullptr;\n" +
		inset + "}\n\n")

	source += (
		inset + "Jimara::Reference<" + reg_class_name + "> " + reg_class_name + "::Instance() {\n" +
		inset + "\tstd::unique_lock<std::mutex> lock(" + registration_lock_name + ");\n" +
		inset + "\tJimara::Reference<" + reg_class_name + "> instance = " + registration_instance_name + ";\n" +
		inset + "\tif(instance == nullptr) {\n" + 
		inset + "\t\tinstance = " + registration_instance_name + " = new " + reg_class_name + "();\n" +
		inset + "\t\t" + registration_instance_name + "->ReleaseRef();\n" +
		inset + "\t}\n" +
		inset + "\treturn instance;\n" +
		inset + "}\n\n")

	all_types = []
	for src_file in job_args.src_files:
		for reg_type in src_file.types:
			all_types.append(reg_type)
	all_types = [t for t in set(all_types)]
	all_types.sort()

	source += inset + reg_class_name + "::" + reg_class_name + "() {\n"
	inset += '\t'
	for reg_type in all_types:
		source += inset + reg_type + "::RegisterType();\n"
		source += inset + "m_typeRegistrationTokens.push_back(Jimara::TypeId::Of<" + reg_type + ">().Register());\n"
	inset = inset[1:]
	source += inset + "}\n\n"

	source += inset + "void " + reg_class_name + "::OnOutOfScope()const {\n"
	inset += '\t'
	source += inset + "std::unique_lock<std::mutex> lock(" + registration_lock_name + ");\n"
	for reg_type in all_types:
		source += inset + reg_type + "::UnregisterType();\n"
	source += (
		inset + registration_instance_name + " = nullptr;\n" +
		inset + "Object::OnOutOfScope();\n")
	inset = inset[1:]
	source += inset + "}\n"
	
	for i in range(len(job_args.reg_class) - 1):
		inset = inset[1:]
		source += inset + "}\n"
	return source


if __name__ == "__main__":
	if len(sys.argv) < 4:
		print(instructions)
	else:
		job_args = job_arguments()
		print(job_args)
		source = generate_source_text(job_args)
		jimara_file_tools.update_text_file(job_args.reg_impl_header, source)
