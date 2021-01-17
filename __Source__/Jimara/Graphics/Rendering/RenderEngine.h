#pragma once
namespace Jimara {
	namespace Graphics {
		class SurfaceRenderEngineInfo;
		class SurfaceRenderEngine;
		class ImageRenderer;
	}
}
#include "../GraphicsDevice.h"
#include "../GraphicsSettings.h"

namespace Jimara {
	namespace Graphics {
		class SurfaceRenderEngineInfo {
		public:
			inline virtual ~SurfaceRenderEngineInfo() {}

			virtual GraphicsDevice* Device()const = 0;

			virtual glm::uvec2 SurfaceSize()const = 0;

			//inline OS::Logger* Log()const { return Device()->Log(); }

		private:
			SurfaceRenderEngineInfo(const SurfaceRenderEngineInfo&) = delete;
			SurfaceRenderEngineInfo& operator=(const SurfaceRenderEngineInfo&) = delete;
		};

		class ImageRenderer : public virtual Object {
		public:

		};

		class SurfaceRenderEngine : public virtual Object {
		public:
			virtual void Update() = 0;
		};
	}
}
