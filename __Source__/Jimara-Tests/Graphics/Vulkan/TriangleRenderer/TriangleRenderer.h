#pragma once
#include "Graphics/Vulkan/Rendering/VulkanRenderEngine.h"
#include "Core/Stopwatch.h"
#include "Data/Mesh.h"
#include <thread>

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

				/// <summary> Virtual destructor </summary>
				virtual ~TriangleRenderer();

				/// <summary>
				/// Creates RenderEngine-specific data (normally requested exclusively by VulkanRenderEngine objects)
				/// </summary>
				/// <param name="engineInfo"> Render engine information </param>
				/// <returns> New instance of an EngineData object </returns>
				virtual Reference<EngineData> CreateEngineData(VulkanRenderEngineInfo* engineInfo) override;

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

				const std::vector<Reference<TriMesh>>& Meshes()const;

				Texture* BearTexture()const;

			protected:
				/// <summary>
				/// Should record all rendering commands via commandRecorder
				/// </summary>
				/// <param name="engineData"> RenderEngine-specific data </param>
				/// <param name="commandRecorder"> Command recorder </param>
				virtual void Render(EngineData* engineData, VulkanCommandRecorder* commandRecorder) override;


			private:
				Reference<VulkanDevice> m_device;
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

				std::vector<Reference<TriMesh>> m_meshes;

				Reference<ImageTexture> m_bearTexture;
			};
		}
	}
}
