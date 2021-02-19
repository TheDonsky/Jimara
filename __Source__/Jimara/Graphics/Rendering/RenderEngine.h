#pragma once
namespace Jimara {
	namespace Graphics {
		class RenderEngineInfo;
		class RenderEngine;
		class ImageRenderer;
	}
}
#include "../GraphicsDevice.h"

namespace Jimara {
	namespace Graphics {
		/// <summary> Exposes basic information about some render engine without keeping any strong references to it </summary>
		class RenderEngineInfo : public virtual Object {
		public:
			/// <summary> Default constructor </summary>
			inline RenderEngineInfo() {}

			/// <summary> Virtual destructor </summary>
			inline virtual ~RenderEngineInfo() {}

			/// <summary> "Owner" graphics device </summary>
			virtual GraphicsDevice* Device()const = 0;

			/// <summary> Render target size </summary>
			virtual Size2 ImageSize()const = 0;

			/// <summary> Render target format </summary>
			virtual Texture::PixelFormat ImageFormat()const = 0;

			/// <summary> Render target image count </summary>
			virtual size_t ImageCount()const = 0;

			/// <summary> 
			/// Render terget by image index 
			/// </summary>
			/// <param name="imageId"> Image index </param>
			/// <returns> Render target </returns>
			virtual Texture* Image(size_t imageId)const = 0;


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
			/// <summary>
			/// Should create an object that stores arbitrary data needed for rendering to some RenderEngine's frame buffers,
			/// that will later be passed into Render() callback, whenever the engine requires a new frame;
			/// </summary>
			/// <param name="engineInfo"></param>
			/// <returns></returns>
			virtual Reference<Object> CreateEngineData(RenderEngineInfo* engineInfo) = 0;

			/// <summary>
			/// Should render an image
			/// </summary>
			/// <param name="engineData"> Engine data, previously created via CreateEngineData() call [Stays consistent per RenderEngine] </param>
			/// <param name="bufferInfo"> Command buffer and target image index information </param>
			virtual void Render(Object* engineData, Pipeline::CommandBufferInfo bufferInfo) = 0;
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
