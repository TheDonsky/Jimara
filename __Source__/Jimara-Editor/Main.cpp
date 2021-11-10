#include "Environment/JimaraEditor.h"

int main(int argc, char* argv[]) {
	Jimara::Reference<Jimara::Editor::JimaraEditor> editor = Jimara::Editor::JimaraEditor::Create();
	if (editor == nullptr) return 1;
	editor->WaitTillClosed();
	return 0;
}
