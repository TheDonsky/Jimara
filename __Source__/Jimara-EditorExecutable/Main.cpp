#include <Jimara-Editor/Environment/JimaraEditor.h>
#include <Jimara/OS/Logging/StreamLogger.h>

#ifdef __APPLE__
#include <Jimara/OS/System/MainThreadCallbacks.h>
#endif


int main(int argc, char* argv[]) {
	#ifdef __APPLE__
    return Jimara::OS::MainThreadCallbacks::RunOnSecondaryThread([](int argc, char** argv) {
	#endif
	Jimara::Editor::JimaraEditor::CreateArgs args = {};
	for (int i = 1; i < argc; i++) {
		const std::string_view& arg = argv[i];
		size_t pos = arg.find('=');
		if (pos == std::string_view::npos) continue;
		const std::string_view param = arg.substr(0, pos);
		const std::string_view value = arg.substr(pos + 1u);
		if (param == "-graphics_device") {
			args.graphicsDeviceIndex = static_cast<size_t>(std::atoi(value.data()));
			Jimara::OS::StreamLogger().Info("graphics_device = ", args.graphicsDeviceIndex.value());
		}
		else if (param == "-asset_directory") {
			args.assetDirectory = value;
			Jimara::OS::StreamLogger().Info("asset_directory = ", args.assetDirectory);
		}
	}
	Jimara::Reference<Jimara::Editor::JimaraEditor> editor = Jimara::Editor::JimaraEditor::Create(args);
	if (editor == nullptr) return 1;
	editor->WaitTillClosed();
	return 0;
	#ifdef __APPLE__
    }, argc, argv);
	#endif
}
