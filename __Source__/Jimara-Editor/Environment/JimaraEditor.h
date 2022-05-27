#pragma once
#include <Environment/Scene/Scene.h>
#include <Environment/Rendering/LightingModels/LightingModel.h>
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
#include "../ActionManagement/UndoStack.h"
#include "JimaraEditorTypeRegistry.h"
#include "EditorScene.h"
#include "EditorInput.h"

namespace Jimara {
	namespace Editor {
		class EditorContext : public virtual Object {
		public:
			inline OS::Logger* Log()const { return m_logger; }

			inline Graphics::GraphicsDevice* GraphicsDevice()const { return m_graphicsDevice; }

			inline Graphics::ShaderLoader* ShaderBinaryLoader()const { return m_shaderLoader; }

			inline Physics::PhysicsInstance* PhysicsInstance()const { return m_physicsInstance; }

			inline Audio::AudioDevice* AudioDevice()const { return m_audioDevice; }

			inline OS::Input* InputModule()const { return m_inputModule; }

			Reference<EditorInput> CreateInputModule()const;

			struct SceneLightTypes {
				const std::unordered_map<std::string, uint32_t>* lightTypeIds = nullptr;
				size_t perLightDataSize = 0;
			};

			SceneLightTypes LightTypes()const;

			LightingModel* DefaultLightingModel()const;

			FileSystemDatabase* EditorAssetDatabase()const;

			void AddRenderJob(JobSystem::Job* job);

			void RemoveRenderJob(JobSystem::Job* job);

			Reference<EditorScene> GetScene()const;

			void SetScene(EditorScene* scene);

			Event<Reference<EditorScene>, const EditorContext*>& OnSceneChanged()const;

			void AddUndoAction(UndoStack::Action* action)const;

			void AddStorageObject(Object* object);

			void RemoveStorageObject(Object* object);


		private:
			const Reference<OS::Logger> m_logger;
			const Reference<Graphics::GraphicsDevice> m_graphicsDevice;
			const Reference<Graphics::ShaderLoader> m_shaderLoader;
			const Reference<Physics::PhysicsInstance> m_physicsInstance;
			const Reference<Audio::AudioDevice> m_audioDevice;
			const Reference<OS::Input> m_inputModule;
			const Reference<FileSystemDatabase> m_fileSystemDB;
			mutable SpinLock m_editorLock;
			JimaraEditor* m_editor = nullptr;
			mutable EventInstance<Reference<EditorScene>, const EditorContext*> m_onSceneChanged;

			EditorContext(
				OS::Logger* logger,
				Graphics::GraphicsDevice* graphicsDevice, Graphics::ShaderLoader* shaderLoader,
				Physics::PhysicsInstance* physicsInstance,
				Audio::AudioDevice* audioDevice,
				OS::Input* inputModule, 
				FileSystemDatabase* database);

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
			const Reference<UndoStack> m_undoManager = Object::Instantiate<UndoStack>();
			std::vector<Reference<UndoStack::Action>> m_undoActions;

			std::unordered_set<Reference<Object>> m_editorStorage;

			JimaraEditor(
				std::vector<Reference<Object>>&& typeRegistries, EditorContext* context, OS::Window* window,
				Graphics::RenderEngine* renderEngine, Graphics::ImageRenderer* renderer);

			void OnUpdate(OS::Window*);

			friend class EditorContext;
		};

		class EditorMainMenuAction : public virtual Object {
		public:
			EditorMainMenuAction(const std::string_view& menuPath, const std::string_view& tooltip);

			const std::string& MenuPath()const;

			const std::string& Tooltip()const;

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
			const std::string m_tooltip;
		};

		class EditorMainMenuCallback : public virtual EditorMainMenuAction {
		public:
			EditorMainMenuCallback(const std::string_view& menuPath, const std::string_view& tooltip, const Callback<EditorContext*>& action);

			virtual void Execute(EditorContext* context)const override;

		private:
			const Callback<EditorContext*> m_action;
		};
	}
}
