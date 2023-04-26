#pragma once
#include "../../Data/Mesh.h"
#include "../GraphicsDevice.h"
#include "../../Core/Synch/SpinLock.h"
#include <mutex>


namespace Jimara {
	namespace Graphics {
		/// <summary>
		/// TriMesh representation as graphics buffers
		/// </summary>
		class JIMARA_API GraphicsMesh : public virtual Object {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="device"> Graphics device </param>
			/// <param name="mesh"> Triangulated mesh </param>
			/// <param name="geometryType"> Type of geometry (TRIANGLE/EDGE) </param>
			GraphicsMesh(GraphicsDevice* device, const TriMesh* mesh, Experimental::GraphicsPipeline::IndexType geometryType);

			/// <summary> Virtual destructor </summary>
			virtual ~GraphicsMesh();

			/// <summary>
			/// Cached instance of the graphics mesh
			/// </summary>
			/// <param name="device"> Graphics device </param>
			/// <param name="mesh"> Triangulated mesh </param>
			/// <param name="geometryType"> Type of geometry (TRIANGLE/EDGE) </param>
			/// <returns> GraphicsMesh instance </returns>
			static Reference<GraphicsMesh> Cached(GraphicsDevice* device, const TriMesh* mesh, Experimental::GraphicsPipeline::IndexType geometryType);

			/// <summary>
			/// Atomically fetches buffers
			/// </summary>
			/// <param name="vertexBuffer"> Vertex buffer reference's reference </param>
			/// <param name="indexBuffer"> Index buffer reference's reference </param>
			void GetBuffers(ArrayBufferReference<MeshVertex>& vertexBuffer, ArrayBufferReference<uint32_t>& indexBuffer);

			/// <summary> Invoked, whenever the underlying mesh gets altered and buffers are no longer up to date </summary>
			Event<GraphicsMesh*>& OnInvalidate();



		private:
			// Graphics device
			const Reference<GraphicsDevice> m_device;
			
			// Mesh
			const Reference<const TriMesh> m_mesh;

			// Type of geometry
			const Experimental::GraphicsPipeline::IndexType m_indexType;

			// Current vertex buffer
			ArrayBufferReference<MeshVertex> m_vertexBuffer;

			// Current index buffer
			ArrayBufferReference<uint32_t> m_indexBuffer;

			// Lock for updating buffers
			SpinLock m_bufferLock;

			// Invoked, whenever the underlying mesh gets altered and buffers are no longer up to date
			EventInstance<GraphicsMesh*> m_onInvalidate;

			// Mesh change callback
			void MeshChanged(const Mesh<MeshVertex, TriangleFace>* mesh);
		};
	}
}
