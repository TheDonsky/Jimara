#pragma once
#include <Jimara/Environment/Scene/Scene.h>
#include <Jimara/Environment/Rendering/LightingModels/LightingModel.h>
#include <Jimara/OS/Window/Window.h>
#include <Jimara/Data/AssetDatabase/FileSystemDatabase/FileSystemDatabase.h>
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
		class JIMARA_EDITOR_API EditorContext : public virtual Object {
		public:
			inline OS::Logger* Log()const { return m_logger; }

			inline Graphics::GraphicsDevice* GraphicsDevice()const { return m_graphicsDevice; }

			inline Graphics::BindlessSet<Graphics::ArrayBuffer>* BindlessBuffers()const { return m_bindlessBuffers; }

			inline Graphics::BindlessSet<Graphics::TextureSampler>* BindlessSamplers()const { return m_bindlessSamplers; }

			inline ::Jimara::ShaderLibrary* ShaderLibrary()const { return m_shaderLibrary; }

			inline Physics::PhysicsInstance* PhysicsInstance()const { return m_physicsInstance; }

			inline Audio::AudioDevice* AudioDevice()const { return m_audioDevice; }

			inline OS::Window* Window()const { return m_window; }

			inline OS::Input* InputModule()const { return m_inputModule; }

			Reference<EditorInput> CreateInputModule()const;

			LightingModel* DefaultLightingModel()const;

			FileSystemDatabase* EditorAssetDatabase()const;

			void AddRenderJob(JobSystem::Job* job);

			void RemoveRenderJob(JobSystem::Job* job);

			Event<>& OnMainLoop();

			Reference<EditorScene> GetScene()const;

			void SetScene(EditorScene* scene);

			Event<Reference<EditorScene>, const EditorContext*>& OnSceneChanged()const;

			void AddUndoAction(UndoStack::Action* action)const;

			void AddStorageObject(Object* object);

			void RemoveStorageObject(Object* object);


		private:
			const Reference<OS::Logger> m_logger;
			const Reference<Graphics::GraphicsDevice> m_graphicsDevice;
			const Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>> m_bindlessBuffers;
			const Reference<Graphics::BindlessSet<Graphics::TextureSampler>> m_bindlessSamplers;
			const Reference<Physics::PhysicsInstance> m_physicsInstance;
			const Reference<Audio::AudioDevice> m_audioDevice;
			const Reference<OS::Input> m_inputModule;
			const Reference<FileSystemDatabase> m_fileSystemDB;
			const Reference<::Jimara::ShaderLibrary> m_shaderLibrary;
			const Reference<OS::Window> m_window;
			
			EventInstance<> m_onMainLoopUpdate;

			mutable SpinLock m_editorLock;
			JimaraEditor* m_editor = nullptr;
			mutable EventInstance<Reference<EditorScene>, const EditorContext*> m_onSceneChanged;

			EditorContext(
				OS::Logger* logger,
				Graphics::GraphicsDevice* graphicsDevice,
				Graphics::BindlessSet<Graphics::ArrayBuffer>* bindlessBuffers,
				Graphics::BindlessSet<Graphics::TextureSampler>* bindlessSamplers,
				Physics::PhysicsInstance* physicsInstance,
				Audio::AudioDevice* audioDevice,
				OS::Input* inputModule, 
				FileSystemDatabase* database, 
				::Jimara::ShaderLibrary* shaderLibrary,
				OS::Window* window);

			friend class JimaraEditor;
		};

		class JIMARA_EDITOR_API JimaraEditor : public virtual Object {
		public:
			struct CreateArgs {
				Reference<Graphics::GraphicsInstance> graphicsInstance;
				Reference<Physics::PhysicsInstance> physicsInstance;
				Reference<Audio::AudioDevice> audioDevice;
				Reference<OS::Window> targetWindow;
				std::optional<size_t> graphicsDeviceIndex;
				OS::Path assetDirectory;
			};


			static Reference<JimaraEditor> Create(const CreateArgs& args = CreateArgs());

			virtual ~JimaraEditor();

			void WaitTillClosed()const;

		private:
			const std::vector<Reference<Object>> m_typeRegistries;
			const Reference<EditorContext> m_context;
			const Reference<Graphics::RenderEngine> m_renderEngine;
			const Reference<Graphics::ImageRenderer> m_renderer;
			const Reference<OS::DirectoryChangeObserver> m_gameLibraryObserver;
			std::mutex m_updateLock;

			std::vector<Reference<Object>> m_gameLibraries;
			Reference<EditorScene> m_scene;
			JobSystem m_jobs = JobSystem(1);
			Reference<UndoStack> m_undoManager = Object::Instantiate<UndoStack>();
			std::vector<Reference<UndoStack::Action>> m_undoActions;
			std::unordered_set<Reference<Object>> m_editorStorage;

			JimaraEditor(
				std::vector<Reference<Object>>&& typeRegistries, EditorContext* context,
				Graphics::RenderEngine* renderEngine, Graphics::ImageRenderer* renderer,
				OS::DirectoryChangeObserver* gameLibraryObserver);

			void OnUpdate(OS::Window*);
			void OnGameLibraryUpdated(const OS::DirectoryChangeObserver::FileChangeInfo& info);

			friend class EditorContext;
		};

		class JIMARA_EDITOR_API EditorMainMenuAction : public virtual Object {
		public:
			EditorMainMenuAction(const std::string_view& menuPath, const std::string_view& tooltip);

			const std::string& MenuPath()const;

			const std::string& Tooltip()const;

			virtual void Execute(EditorContext* context)const = 0;

			static void GetAll(Callback<const EditorMainMenuAction*> recordEntry);

			template<typename RecordCallback>
			static void GetAll(const RecordCallback& callback) {
				void (*call)(const RecordCallback * c, const EditorMainMenuAction * i) = [](const RecordCallback* c, const EditorMainMenuAction* i) { (*c)(i); };
				GetAll(Callback<const EditorMainMenuAction*>(call, &callback));
			}

		private:
			const std::string m_path;
			const std::string m_tooltip;
		};

		class JIMARA_EDITOR_API EditorMainMenuCallback : public virtual EditorMainMenuAction {
		public:
			EditorMainMenuCallback(const std::string_view& menuPath, const std::string_view& tooltip, const Callback<EditorContext*>& action);

			virtual void Execute(EditorContext* context)const override;

		private:
			const Callback<EditorContext*> m_action;
		};
	}
}
