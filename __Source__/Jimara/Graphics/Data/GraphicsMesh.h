#pragma once
#include "../../Data/Mesh.h"
#include "../GraphicsDevice.h"
#include <mutex>


namespace Jimara {
	namespace Graphics {
		class GraphicsMesh : public virtual ObjectCache<TriMesh*>::StoredObject {
		public:
			GraphicsMesh(GraphicsDevice* device, TriMesh* mesh);

			virtual ~GraphicsMesh();

			void GetBuffers(ArrayBufferReference<MeshVertex>& vertexBuffer, ArrayBufferReference<uint32_t>& indexBuffer);

			Event<GraphicsMesh*>& OnInvalidate();

		private:
			const Reference<GraphicsDevice> m_device;
			
			const Reference<TriMesh> m_mesh;

			ArrayBufferReference<MeshVertex> m_vertexBuffer;

			ArrayBufferReference<uint32_t> m_indexBuffer;

			std::atomic<uint64_t> m_revision;

			std::recursive_mutex m_bufferLock;

			EventInstance<GraphicsMesh*> m_onInvalidate;

			void OnMeshChanged(Mesh<MeshVertex, TriangleFace>* mesh);
		};

		class GraphicsMeshCache : public virtual ObjectCache<TriMesh*> {
		public:
			GraphicsMeshCache(GraphicsDevice* device);

			Reference<GraphicsMesh> GetMesh(TriMesh* mesh, bool storePermanently);

		private:
			const Reference<GraphicsDevice> m_device;
		};
	}
}
