#include <Jimara-Editor/Environment/JimaraEditor.h>


int main(int argc, char* argv[]) {
	Jimara::Editor::JimaraEditor::CreateArgs args = {};
	for (int i = 1; i < argc; i++) {
		const std::string_view& arg = argv[i];
		size_t pos = arg.find('=');
		if (pos == std::string_view::npos) continue;
		const std::string_view param = arg.substr(0, pos);
		const std::string_view value = arg.substr(pos + 1u);
		if (param == "-graphics_device")
			args.graphicsDeviceIndex = static_cast<size_t>(std::atoi(value.data()));
	}
	Jimara::Reference<Jimara::Editor::JimaraEditor> editor = Jimara::Editor::JimaraEditor::Create(args);
	if (editor == nullptr) return 1;
	editor->WaitTillClosed();
	return 0;
}
