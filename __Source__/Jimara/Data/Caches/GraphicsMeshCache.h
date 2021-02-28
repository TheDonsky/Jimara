#pragma once
#include "../Mesh.h"
#include "../../Graphics/GraphicsDevice.h"
#include <mutex>


namespace Jimara {
	namespace Cache {
		class GraphicsMesh : public virtual ObjectCache<TriMesh*>::StoredObject {
		public:
			GraphicsMesh(Graphics::GraphicsDevice* device, TriMesh* mesh);

			virtual ~GraphicsMesh();

			void GetBuffers(Graphics::ArrayBufferReference<MeshVertex>& vertexBuffer, Graphics::ArrayBufferReference<uint32_t>& indexBuffer);

			Event<GraphicsMesh*>& OnInvalidate();

		private:
			const Reference<Graphics::GraphicsDevice> m_device;
			
			const Reference<TriMesh> m_mesh;

			Graphics::ArrayBufferReference<MeshVertex> m_vertexBuffer;

			Graphics::ArrayBufferReference<uint32_t> m_indexBuffer;

			std::atomic<uint64_t> m_revision;

			std::recursive_mutex m_bufferLock;

			EventInstance<GraphicsMesh*> m_onInvalidate;

			void OnMeshChanged(Mesh<MeshVertex, TriangleFace>* mesh);
		};

		class GraphicsMeshCache : public virtual ObjectCache<TriMesh*> {
		public:
			GraphicsMeshCache(Graphics::GraphicsDevice* device);

			Reference<GraphicsMesh> GetMesh(TriMesh* mesh, bool storePermanently);

		private:
			const Reference<Graphics::GraphicsDevice> m_device;
		};
	}
}
