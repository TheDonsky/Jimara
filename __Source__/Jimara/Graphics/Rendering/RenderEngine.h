#pragma once
namespace Jimara {
	namespace Graphics {
		class RenderEngineInfo;
		class RenderEngine;
		class ImageRenderer;
	}
}
#include "../GraphicsDevice.h"
#include "../GraphicsSettings.h"

namespace Jimara {
	namespace Graphics {
		/// <summary> Exposes basic information about some render engine without keeping any strong references to it </summary>
		class RenderEngineInfo {
		public:
			/// <summary> Default constructor </summary>
			inline RenderEngineInfo() {}

			/// <summary> Virtual destructor </summary>
			inline virtual ~RenderEngineInfo() {}

			/// <summary> "Owner" graphics device </summary>
			virtual GraphicsDevice* Device()const = 0;

			/// <summary> Render target size </summary>
			virtual glm::uvec2 TargetSize()const = 0;

			/// <summary> Logger </summary>
			OS::Logger* Log()const;

		private:
			// We don't need any copy-pasting here...
			RenderEngineInfo(const RenderEngineInfo&) = delete;
			RenderEngineInfo& operator=(const RenderEngineInfo&) = delete;
		};

		/// <summary> 
		/// Generic renderer that can be added to a render engine.
		/// Can be simultinuously used with more than one render egines.
		/// </summary>
		class ImageRenderer : public virtual Object {
		public:

		};

		/// <summary>
		/// Render engine, that can drive render process for something like a window surface (but not necessarily a surface)
		/// </summary>
		class RenderEngine : public virtual Object {
		public:
			/// <summary> Invokes all underlying image renderers for the target </summary>
			virtual void Update() = 0;

			/// <summary>
			/// Adds an ImageRenderer to the engine
			/// </summary>
			/// <param name="renderer"> Renderer to add </param>
			virtual void AddRenderer(ImageRenderer* renderer) = 0;

			/// <summary>
			/// Removes an image renderere for the engine
			/// </summary>
			/// <param name="renderer"> Renderer to remove </param>
			virtual void RemoveRenderer(ImageRenderer* renderer) = 0;
		};
	}
}
