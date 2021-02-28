#pragma once
#include "Graphics/Rendering/RenderEngine.h"
#include "Data/Caches/GraphicsMeshCache.h"
#include "Core/Stopwatch.h"
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

				/// <summary> Shader cache </summary>
				Graphics::ShaderCache* ShaderCache()const;

				/// <summary> Camera transform constant buffer </summary>
				Buffer* CameraTransform()const;

				/// <summary> Point light descriptor </summary>
				struct Light {
					alignas(16) Vector3 position;
					alignas(16) Vector3 color;
				};

				/// <summary> Lights buffer </summary>
				ArrayBufferReference<Light> Lights()const;

				/// <summary> Cbuffer </summary>
				Buffer* ConstantBuffer()const;

				/// <summary> Triangle texture sampler </summary>
				TextureSampler* Sampler()const;

				/// <summary> Vertex position buffer </summary>
				VertexBuffer* PositionBuffer();

				/// <summary> Instance position offset buffer </summary>
				InstanceBuffer* InstanceOffsetBuffer();


				Cache::GraphicsMeshCache* GraphicsMeshCache()const;

				const std::vector<Reference<TriMesh>>& Meshes()const;

				Texture* BearTexture()const;

			protected:
				/// <summary>
				/// Should record all rendering commands via commandRecorder
				/// </summary>
				/// <param name="engineData"> RenderEngine-specific data </param>
				/// <param name="bufferInfo"> Command buffer info </param>
				virtual void Render(Object* engineData, Pipeline::CommandBufferInfo bufferInfo) override;


			private:
				Reference<GraphicsDevice> m_device;
				Reference<Graphics::ShaderCache> m_shaderCache;

				BufferReference<Matrix4> m_cameraTransform;

				ArrayBufferReference<Light> m_lights;

				BufferReference<float> m_cbuffer;

				Reference<ImageTexture> m_texture;
				Reference<TextureSampler> m_sampler;

				class VertexPositionBuffer : public virtual VertexBuffer {
				private:
					ArrayBufferReference<Vector2> m_buffer;

				public:
					VertexPositionBuffer(GraphicsDevice* device);

					virtual Reference<ArrayBuffer> Buffer() override;

					virtual size_t AttributeCount()const override;

					virtual AttributeInfo Attribute(size_t index)const override;

					virtual size_t BufferElemSize()const override;
				} m_positionBuffer;

				class InstanceOffsetBuffer : public virtual InstanceBuffer {
				private:
					ArrayBufferReference<Vector2> m_buffer;

				public:
					InstanceOffsetBuffer(GraphicsDevice* device);

					virtual Reference<ArrayBuffer> Buffer() override;

					virtual size_t AttributeCount()const override;

					virtual AttributeInfo Attribute(size_t index)const override;

					virtual size_t BufferElemSize()const override;
				} m_instanceOffsetBuffer;


				volatile bool m_rendererAlive;

				std::thread m_imageUpdateThread;

				Stopwatch m_stopwatch;
				
				Reference<Cache::GraphicsMeshCache> m_graphicsMeshCache;

				std::vector<Reference<TriMesh>> m_meshes;

				Reference<ImageTexture> m_bearTexture;
			};
		}
	}
}
