#pragma once
#include <Environment/Scene.h>
#include <Core/Systems/JobSystem.h>
#include <Core/Synch/SpinLock.h>
#include <OS/Window/Window.h>
#include <Data/AssetDatabase/FileSystemDatabase/FileSystemDatabase.h>
namespace Jimara {
	namespace Editor {
		class EditorContext;
		class JimaraEditor;
		class EditorMainMenuAction;
		class EditorMainMenuCallback;
	}
}
#include "JimaraEditorTypeRegistry.h"
#include "EditorScene.h"

namespace Jimara {
	namespace Editor {
		class EditorContext : public virtual Object {
		public:
			AppContext* ApplicationContext()const;

			Graphics::ShaderLoader* ShaderBinaryLoader()const;

			OS::Input* InputModule()const;

			struct SceneLightTypes {
				const std::unordered_map<std::string, uint32_t>* lightTypeIds = nullptr;
				size_t perLightDataSize = 0;
			};

			SceneLightTypes LightTypes()const;

			LightingModel* DefaultLightingModel()const;

			void AddRenderJob(JobSystem::Job* job);

			void RemoveRenderJob(JobSystem::Job* job);

			Reference<EditorScene> GetScene()const;

			void SetScene(EditorScene* scene);

			Event<Reference<EditorScene>, const EditorContext*>& OnSceneChanged()const;



		private:
			const Reference<AppContext> m_appContext;
			const Reference<Graphics::ShaderLoader> m_shaderLoader;
			const Reference<OS::Input> m_inputModule;
			const Reference<FileSystemDatabase> m_fileSystemDB;
			mutable SpinLock m_editorLock;
			JimaraEditor* m_editor = nullptr;
			mutable EventInstance<Reference<EditorScene>, const EditorContext*> m_onSceneChanged;

			EditorContext(AppContext* appContext, Graphics::ShaderLoader* shaderLoader, OS::Input* inputModule, FileSystemDatabase* database);

			friend class JimaraEditor;
		};

		class JimaraEditor : public virtual Object {
		public:
			static Reference<JimaraEditor> Create(
				Graphics::GraphicsInstance* graphicsInstance = nullptr, 
				Physics::PhysicsInstance* physicsInstance = nullptr,
				Audio::AudioDevice* audioDevice = nullptr,
				OS::Window* targetWindow = nullptr);

			virtual ~JimaraEditor();

			void WaitTillClosed()const;

		private:
			const std::vector<Reference<Object>> m_typeRegistries;
			const Reference<EditorContext> m_context;
			const Reference<OS::Window> m_window;
			const Reference<Graphics::RenderEngine> m_renderEngine;
			const Reference<Graphics::ImageRenderer> m_renderer;

			Reference<EditorScene> m_scene;
			JobSystem m_jobs = JobSystem(1);

			JimaraEditor(
				std::vector<Reference<Object>>&& typeRegistries, EditorContext* context, OS::Window* window,
				Graphics::RenderEngine* renderEngine, Graphics::ImageRenderer* renderer);

			void OnUpdate(OS::Window*);

			friend class EditorContext;
		};

		class EditorMainMenuAction : public virtual Object {
		public:
			EditorMainMenuAction(const std::string_view& menuPath);

			virtual const std::string& MenuPath()const;

			virtual void Execute(EditorContext* context)const = 0;

			class RegistryEntry : public virtual Object {
			public:
				RegistryEntry(const EditorMainMenuAction* instancer = nullptr);

				virtual ~RegistryEntry();

				operator const EditorMainMenuAction* ()const;

				EditorMainMenuAction::RegistryEntry& operator=(const EditorMainMenuAction* instancer);

				EditorMainMenuAction::RegistryEntry& operator=(const EditorMainMenuAction::RegistryEntry& entry);

				static void GetAll(Callback<const EditorMainMenuAction*> recordEntry);

				template<typename RecordCallback>
				static void GetAll(const RecordCallback& callback) {
					void (*call)(const RecordCallback * c, const EditorMainMenuAction * i) = [](const RecordCallback* c, const EditorMainMenuAction* i) { (*c)(i); };
					GetAll(Callback<const EditorMainMenuAction*>(call, &callback));
				}

			private:
				Reference<const EditorMainMenuAction> m_action;
			};

		private:
			const std::string m_path;
		};

		class EditorMainMenuCallback : public virtual EditorMainMenuAction {
		public:
			EditorMainMenuCallback(const std::string_view& menuPath, const Callback<EditorContext*>& action);

			virtual void Execute(EditorContext* context)const override;

		private:
			const Callback<EditorContext*> m_action;
		};
	}
}
