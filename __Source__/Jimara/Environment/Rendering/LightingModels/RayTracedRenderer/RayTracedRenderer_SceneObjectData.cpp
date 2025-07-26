#include "RayTracedRenderer_Tools.h"


namespace Jimara {
	struct RayTracedRenderer::Tools::SceneObjectData::Helpers {
		static_assert(sizeof(RayTracedRenderer::Tools::PerObjectData) == 144u);
		static_assert(alignof(RayTracedRenderer::Tools::PerObjectData) == 8u);

		inline static void ClearResources(SceneObjectData* self) {
			self->m_rasterizedGeometryResources.clear();
			self->m_perObjectDataBinding->BoundObject() = nullptr;
		}

		inline static bool UpdateRasterizedGeometryResources(SceneObjectData* self, const GraphicsObjectPipelines::Reader& rasterPipelines, RayTracedPass* rtPass) {
			
			auto fail = [&](const auto... message) {
				ClearResources(self);
				self->m_context->Log()->Error("RayTracedRenderer::Tools::SceneObjectData::UpdateRasterizedGeometryResources - ", message...);
				return false;
			};

			// Roghly speaking, we only need to keep only those rasterized-geometry resources 'alive', that are within raster pipelines:
			if (self->m_rasterizedGeometryResources.size() > rasterPipelines.Count())
				self->m_rasterizedGeometryResources.resize(rasterPipelines.Count());
			else while (self->m_rasterizedGeometryResources.size() < rasterPipelines.Count())
				self->m_rasterizedGeometryResources.push_back({});

			// Update all rasterized resources:
			for (size_t i = 0u; i < self->m_rasterizedGeometryResources.size(); i++) {
				// Obtain viewport data:
				PerObjectResources& resources = self->m_rasterizedGeometryResources[i];
				const GraphicsObjectPipelines::ObjectInfo& info = rasterPipelines[i];
				const IndexedGraphicsObjectDataProvider::ViewportData* viewportData =
					dynamic_cast<const IndexedGraphicsObjectDataProvider::ViewportData*>(info.ViewData());
				if (viewportData == nullptr)
					return fail("Raster pipeline viewport data is not an instance of IndexedGraphicsObjectDataProvider::ViewportData! ",
						"[File: ", __FILE__, "; Line: ", __LINE__, "]");

				// Update lit-shader if there's a need to do so:
				if (self->m_lastRTPipeline != rtPass->Pipeline() || viewportData != resources.vertexInput.Source())
					resources.materialId = rtPass->MaterialIndex(info.Descriptor()->Shader());

				// Update resources:
				if (viewportData != resources.vertexInput.Source()) {
					resources.vertexInput = JM_StandardVertexInput::Extractor(viewportData, self->m_context->Log());
					const Graphics::BindingSet::BindingSearchFunctions searchFunctions = viewportData->BindingSearchFunctions();
					static const Graphics::BindingSet::BindingDescriptor settingsBufferDesc = {
						Material::SETTINGS_BUFFER_BINDING_NAME, 0u, 0u
					};
					resources.materialSettingsBuffer = searchFunctions.structuredBuffer(settingsBufferDesc);
					resources.flags = 0u;
					if (viewportData->GeometryType() == Graphics::GraphicsPipeline::IndexType::EDGE)
						resources.flags |= JM_RT_FLAG_EDGES;
					resources.objectIndex = viewportData->Index();
				}
			}

			// Done:
			return true;
		}

		inline static bool UpdateObjectDataBuffer(SceneObjectData* self, std::vector<Reference<const Object>>& resourceList) {
			auto fail = [&](const auto... message) {
				ClearResources(self);
				self->m_context->Log()->Error("RayTracedRenderer::Tools::SceneObjectData::UpdateObjectDataBuffer - ", message...);
				return false;
			};

			// Find desired buffer size:
			size_t bufferSize = 1u;

			// Make sure all the rasterized-geometry data can fit in the buffer:
			while (bufferSize < self->m_rasterizedGeometryResources.size())
				bufferSize <<= 1u;
			for (size_t i = 0u; i < self->m_rasterizedGeometryResources.size(); i++) {
				const uint32_t minIndirectionSize = self->m_rasterizedGeometryResources[i].objectIndex + 1u;
				while (bufferSize < minIndirectionSize)
					bufferSize <<= 1u;
			}

			// (Re)Create buffer if needed:
			Graphics::ArrayBufferReference<PerObjectData> objectDataBuffer = self->m_perObjectDataBinding->BoundObject();
			if (objectDataBuffer == nullptr || objectDataBuffer->ObjectCount() != bufferSize) {
				objectDataBuffer = self->m_context->Graphics()->Device()->CreateArrayBuffer<PerObjectData>(bufferSize);
				if (objectDataBuffer == nullptr)
					return fail("Failed to allocate PerObjectData-Buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				self->m_perObjectDataBinding->BoundObject() = objectDataBuffer;
			}

			// Tool for setting a buffer device address from resource binding:
			auto getBufferDeviceAddress = [&](uint64_t& value, const Graphics::ResourceBinding<Graphics::ArrayBuffer>* binding, uint64_t defaultValue = 0u) {
				value = defaultValue;
				if (binding == nullptr)
					return;
				Graphics::ArrayBuffer* const buffer = binding->BoundObject();
				if (buffer == nullptr)
					return;
				value = buffer->DeviceAddress();
				resourceList.push_back(buffer);
			};

			// Fill-in the buffer content:
			{
				PerObjectData* objectData = objectDataBuffer.Map();
				if (objectData == nullptr)
					return fail("Failed to map per-object data! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				for (size_t i = 0u; i < bufferSize; i++)
					objectData[i] = {};
				for (size_t i = 0u; i < self->m_rasterizedGeometryResources.size(); i++) {
					const PerObjectResources& resource = self->m_rasterizedGeometryResources[i];
					PerObjectData& data = objectData[resource.objectIndex];
					
					data.vertexInput = resource.vertexInput.Get(&resourceList);
					getBufferDeviceAddress(data.indexBufferId, resource.vertexInput.IndexBuffer(), 0u);

					getBufferDeviceAddress(data.materialSettingsBufferId, resource.materialSettingsBuffer, 0u);
					data.materialId = resource.materialId;

					data.flags = resource.flags;
				}
				objectDataBuffer->Unmap(true);
			}

			// Done:
			return true;
		}
	};

	RayTracedRenderer::Tools::SceneObjectData::SceneObjectData(SharedBindings* sharedBindings)
		: m_context(sharedBindings->viewport->Context())
		, m_perObjectDataBinding(sharedBindings->perObjectDataBinding) {}

	RayTracedRenderer::Tools::SceneObjectData::~SceneObjectData() {}

	bool RayTracedRenderer::Tools::SceneObjectData::Update(
		const GraphicsObjectPipelines::Reader& rasterPipelines, RayTracedPass* rtPass, 
		std::vector<Reference<const Object>>& resourceList) {
		
		if (!Helpers::UpdateRasterizedGeometryResources(this, rasterPipelines, rtPass))
			return false;

		if (!Helpers::UpdateObjectDataBuffer(this, resourceList))
			return false;

		m_lastRTPipeline = rtPass->Pipeline();
		return true;
	}
}
