#pragma once
namespace Jimara {
	namespace Editor {
		class ImGuiRenderer;
	}
}
#include "ImGuiAPIContext.h"
#include <Core/Systems/JobSystem.h>
#include <Graphics/GraphicsDevice.h>

namespace Jimara {
	namespace Editor {
		/// <summary>
		/// ImGui-renderable texture
		/// </summary>
		class ImGuiTexture : public virtual Object {
		public:
			/// <summary> Corresponding ImTextureId </summary>
			virtual operator ImTextureID()const = 0;
		};

		/// <summary>
		/// Editor GUI renderer
		/// Note: This gets recreated each time the properties of the target window change
		/// </summary>
		class ImGuiRenderer : public virtual Object {
		public:
			/// <summary>
			/// Executes the render jobs
			/// Note: This one locks ImGuiAPIContext::APILock and more or less guarantees thread-safe ImGui calls for the underlying jobs
			/// </summary>
			/// <param name="bufferInfo"> Command buffer to record to </param>
			void Render(const Graphics::Pipeline::CommandBufferInfo& bufferInfo);

			/// <summary>
			/// Graphics::Pipeline::CommandBufferInfo passed to the currently executing Render() call;
			/// Note: Only accessible inside the Render() call, where the ImGui commands are accessible
			/// </summary>
			static const Graphics::Pipeline::CommandBufferInfo& BufferInfo();

			/// <summary>
			/// Creates ImGui-drawable texture from Engine texture
			/// </summary>
			/// <param name="sampler"> Texture sampler </param>
			/// <returns> Gui-drawable texture </returns>
			static Reference<ImGuiTexture> Texture(Graphics::TextureSampler* sampler);

			/// <summary> 
			/// Sets 'Field modification' flag
			/// <para /> Note: Invoke this if you have any ImGui field calls returning true, 
			///		or risk Editor's undo system going out of sync with ImGui field modification... 
			/// </summary>
			static void FieldModified();

			/// <summary> Returns true, if FieldModified() was called on this frame at least once so far (for ANY reason) </summary>
			static bool AnyFieldModified();

			/// <summary>
			/// Adds a job to the renderer
			/// </summary>
			/// <param name="job"> Job to add </param>
			void AddRenderJob(JobSystem::Job* job);

			/// <summary>
			/// Removes job from the renderer
			/// </summary>
			/// <param name="job"> Job to remove </param>
			void RemoveRenderJob(JobSystem::Job* job);

		protected:
			/// <summary> Should begin ImGui frame </summary>
			virtual void BeginFrame() = 0;

			/// <summary> Should end ImGui frame </summary>
			virtual void EndFrame() = 0;

			/// <summary>
			/// Creates ImGui-drawable texture from Engine texture
			/// </summary>
			/// <param name="sampler"> Texture sampler </param>
			/// <returns> Gui-drawable texture </returns>
			virtual Reference<ImGuiTexture> CreateTexture(Graphics::TextureSampler* sampler) = 0;

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="deviceContext"> ImGui device context </param>
			inline ImGuiRenderer(ImGuiDeviceContext* deviceContext) : m_deviceContext(deviceContext) { }

		private:
			// Device context
			const Reference<ImGuiDeviceContext> m_deviceContext;

			// Job system
			JobSystem m_jobs = JobSystem(1);
		};
	}
}
