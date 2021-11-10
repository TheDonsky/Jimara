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
		class ImGuiRenderer : public virtual Object {
		public:
			void Render(const Graphics::Pipeline::CommandBufferInfo& bufferInfo);

			static const Graphics::Pipeline::CommandBufferInfo& BufferInfo();

			static void Texture(Graphics::Texture* texture, const Rect& rect);

			static bool MenuAction(const std::string_view& menuPath, const void* actionId);

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
			virtual void BeginFrame() = 0;

			virtual void EndFrame() = 0;

			virtual void DrawTexture(Graphics::Texture* texture, const Rect& rect) = 0;

		private:
			JobSystem m_jobs = JobSystem(1);
		};
	}
}
