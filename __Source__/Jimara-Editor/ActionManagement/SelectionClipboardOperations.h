#pragma once
#include "HotKey.h"
#include "SceneClipboard.h"
#include "SceneSelection.h"


namespace Jimara {
	namespace Editor {
		inline void PerformSelectionClipboardOperations(
			SceneClipboard* clipboard, SceneSelection* selection, const OS::Input* input) {
			if (HotKey::Copy().Check(input)) {
				if (selection->Count() > 0)
					clipboard->CopyComponents(selection->Current());
			}
			else if (HotKey::Cut().Check(input)) {
				const auto currentSelection = selection->Current();
				if (!currentSelection.empty())
					clipboard->CopyComponents(selection->Current());
				for (const auto& component : currentSelection)
					if (!component->Destroyed())
						component->Destroy();
			}
			else if (HotKey::Paste().Check(input)) {
				Component* root = nullptr;
				selection->Iterate([&](Component* component) {
					for (Component* it = component->Parent(); it != nullptr; it = it->Parent())
						if (selection->Contains(it))
							return;
					Component* parent = component->Parent();
					root = (root == nullptr || root == parent) ? parent :
						selection->Context()->RootObject().operator->();
					});
				const auto newInstances = clipboard->PasteComponents(
					(root == nullptr) ? selection->Context()->RootObject().operator->() : root);
				selection->DeselectAll();
				selection->Select(newInstances.data(), newInstances.size());
			}
		}
	}
}
