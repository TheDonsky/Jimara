#pragma once
#include "Graphics/Vulkan/Rendering/VulkanRenderEngine.h"

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class TriangleRenderer : public virtual VulkanImageRenderer {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> "Owner" device </param>
				TriangleRenderer(VulkanDevice* device);

				/// <summary>
				/// Creates RenderEngine-specific data (normally requested exclusively by VulkanRenderEngine objects)
				/// </summary>
				/// <param name="engineInfo"> Render engine information </param>
				/// <returns> New instance of an EngineData object </returns>
				virtual Reference<EngineData> CreateEngineData(VulkanRenderEngineInfo* engineInfo) override;

				/// <summary> Shader cache </summary>
				Graphics::ShaderCache* ShaderCache()const;

				/// <summary> Vertex position buffer </summary>
				VertexBuffer* PositionBuffer();


			protected:
				/// <summary>
				/// Should record all rendering commands via commandRecorder
				/// </summary>
				/// <param name="engineData"> RenderEngine-specific data </param>
				/// <param name="commandRecorder"> Command recorder </param>
				virtual void Render(EngineData* engineData, VulkanRenderEngine::CommandRecorder* commandRecorder) override;


			private:
				Reference<VulkanDevice> m_device;
				Reference<Graphics::ShaderCache> m_shaderCache;

				class VertexPositionBuffer : public virtual VertexBuffer {
				private:
					BufferArrayReference<Vector2> m_buffer;

				public:
					VertexPositionBuffer(GraphicsDevice* device);

					virtual Reference<ArrayBuffer> Buffer()const override;

					virtual size_t AttributeCount()const override;

					virtual AttributeInfo Attribute(size_t index)const override;
				} m_positionBuffer;
			};
		}
	}
}
