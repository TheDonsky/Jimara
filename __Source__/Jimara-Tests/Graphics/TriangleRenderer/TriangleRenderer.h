#pragma once
#include "Graphics/Rendering/RenderEngine.h"
#include "Graphics/Data/GraphicsMesh.h"
#include "Data/Mesh.h"
#include <thread>


namespace Jimara {
	namespace Graphics {
		namespace Test {
			class TriangleRenderer : public virtual ImageRenderer {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> "Owner" device </param>
				TriangleRenderer(GraphicsDevice* device);

				/// <summary> Virtual destructor </summary>
				virtual ~TriangleRenderer();

				/// <summary>
				/// Creates RenderEngine-specific data (normally requested exclusively by VulkanRenderEngine objects)
				/// </summary>
				/// <param name="engineInfo"> Render engine information </param>
				/// <returns> New instance of an engine data object </returns>
				virtual Reference<Object> CreateEngineData(RenderEngineInfo* engineInfo) override;

				/// <summary> ShaderCache </summary>
				ShaderCache* GetShaderCache()const;

				/// <summary> Cbuffer </summary>
				Buffer* ConstantBuffer()const;

				/// <summary> Triangle texture sampler </summary>
				TextureSampler* Sampler()const;

				/// <summary> Vertex position buffer </summary>
				ArrayBuffer* PositionBuffer();

				/// <summary> Instance position offset buffer </summary>
				ArrayBuffer* InstanceOffsetBuffer();


			protected:
				/// <summary>
				/// Should record all rendering commands via commandRecorder
				/// </summary>
				/// <param name="engineData"> RenderEngine-specific data </param>
				/// <param name="bufferInfo"> Command buffer info </param>
				virtual void Render(Object* engineData, InFlightBufferInfo bufferInfo) override;


			private:
				const Reference<GraphicsDevice> m_device;
				const Reference<ShaderCache> m_shaderCache;

				BufferReference<float> m_cbuffer;
				Reference<TextureSampler> m_sampler;
				ArrayBufferReference<Vector2> m_positionBuffer;
				ArrayBufferReference<Vector2> m_instanceOffsetBuffer;

				volatile bool m_rendererAlive;

				std::thread m_imageUpdateThread;
			};
		}
	}
}
