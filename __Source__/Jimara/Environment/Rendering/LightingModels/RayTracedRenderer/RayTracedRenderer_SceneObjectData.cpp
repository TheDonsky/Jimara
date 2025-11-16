#include "RayTracedRenderer_Tools.h"


namespace Jimara {
	struct RayTracedRenderer::Tools::SceneObjectData::Helpers {
		static_assert(sizeof(RayTracedRenderer::Tools::PerObjectData) == 144u);
		static_assert(alignof(RayTracedRenderer::Tools::PerObjectData) == 8u);

		inline static void ClearResources(SceneObjectData* self) {
			self->m_rasterizedGeometryResources.clear();
			self->m_tlasGeometryResources.clear();
			self->m_perObjectDataBinding->BoundObject() = nullptr;
			self->m_rasterizedGeometrySize = 0u;
		}

		inline static void UpdateGeometry(
			const SceneObjectData* self,
			PerObjectResources<JM_StandardVertexInput::Extractor>& resources,
			const GraphicsObjectPipelines::ObjectInfo& objectInfo) {
			if (objectInfo.ViewData() != resources.viewportData) {
				resources.vertexInput = JM_StandardVertexInput::Extractor(objectInfo.ViewData(), self->m_context->Log());
				const IndexedGraphicsObjectDataProvider::ViewportData* viewData = 
					dynamic_cast<const IndexedGraphicsObjectDataProvider::ViewportData*>(objectInfo.ViewData());
				if (viewData == nullptr) {
					self->m_context->Log()->Error("RayTracedRenderer::Tools::SceneObjectData::UpdateGeometry - ",
						"Unexpected viewport data type! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					resources.indirectObjectIndex = 0u;
				}
				else resources.indirectObjectIndex = viewData->Index();
			}
		}

		inline static void UpdateGeometry(
			const SceneObjectData* self,
			PerObjectResources<GraphicsObjectDescriptor::GeometryDescriptor>& resources,
			const GraphicsObjectAccelerationStructure::ObjectInformation& objectInfo) {
			resources.vertexInput = objectInfo.geometry;
			resources.indirectObjectIndex = objectInfo.firstInstanceIndex;
		}

		template<typename GeometryResourceT, typename ObjectInfoT,
			typename GetObjectInfoFn, typename GetObjectDescFn, typename GetViewportDataFn>
		inline static bool UpdateGeometryResources(
			SceneObjectData* self,
			std::vector<PerObjectResources<GeometryResourceT>>& geometryResources,
			const size_t objectCount, const GetObjectInfoFn& getObjectInfo, 
			const GetObjectDescFn& getObjectDesc, const GetViewportDataFn& getViewportData,
			const RayTracedPass::State& rtPass) {

			auto fail = [&](const auto... message) {
				ClearResources(self);
				self->m_context->Log()->Error("RayTracedRenderer::Tools::SceneObjectData::UpdateGeometryResources - ", message...);
				return false;
			};

			// Roghly speaking, we only need to keep only those geometry resources 'alive', that are within the pipeline geometry:
			if (geometryResources.size() > objectCount)
				geometryResources.resize(objectCount);
			else while (geometryResources.size() < objectCount)
				geometryResources.push_back({});

			// Update all rasterized resources:
			for (size_t i = 0u; i < geometryResources.size(); i++) {
				// Obtain viewport data:
				auto& resources = geometryResources[i];
				const ObjectInfoT& info = getObjectInfo(i);
				const GraphicsObjectDescriptor::ViewportData* viewportData = getViewportData(info);
				if (viewportData == nullptr)
					return fail("Viewport data missing! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				const Material::LitShader* shader = getObjectDesc(info)->Shader();

				// Update lit-shader if there's a need to do so:
				if (self->m_lastRTPipeline != rtPass.Pipeline() || viewportData != resources.viewportData)
					resources.materialId = rtPass.MaterialIndex(shader);

				// Update geometry:
				UpdateGeometry(self, resources, info);

				// Update resources:
				if (viewportData != resources.viewportData) {
					resources.viewportData = viewportData;

					const Graphics::BindingSet::BindingSearchFunctions searchFunctions = viewportData->BindingSearchFunctions();
					static const Graphics::BindingSet::BindingDescriptor settingsBufferDesc = {
						Material::SETTINGS_BUFFER_BINDING_NAME, 0u, 0u
					};
					resources.materialSettingsBuffer = searchFunctions.structuredBuffer(settingsBufferDesc);
					
					resources.flags = 0u;
					if (viewportData->GeometryType() == Graphics::GraphicsPipeline::IndexType::EDGE)
						resources.flags |= JM_RT_FLAG_EDGES;
					if ((shader->MaterialFlags() & Material::MaterialFlags::CanDiscard) != Material::MaterialFlags::NONE)
						resources.flags |= JM_RT_FLAG_CAN_DISCARD;
				}

			}

			// Done:
			return true;
		}
		
		inline static bool UpdateRasterizedGeometryResources(
			SceneObjectData* self, 
			const GraphicsObjectPipelines::Reader& rasterPipelines, 
			const RayTracedPass::State& rtPass) {
			return UpdateGeometryResources<JM_StandardVertexInput::Extractor, GraphicsObjectPipelines::ObjectInfo>(
				self, self->m_rasterizedGeometryResources, rasterPipelines.Count(),
				[&](size_t index) -> const GraphicsObjectPipelines::ObjectInfo& { return rasterPipelines[index]; },
				[&](const GraphicsObjectPipelines::ObjectInfo& info) -> const GraphicsObjectDescriptor* { return info.Descriptor(); },
				[&](const GraphicsObjectPipelines::ObjectInfo& info) -> const GraphicsObjectDescriptor::ViewportData* { return info.ViewData(); },
				rtPass);
		}

		inline static bool UpdateRayTracedGeometryResources(
			SceneObjectData* self,
			const RayTracedPass::State& rtPass) {
			using ObjectInfo = GraphicsObjectAccelerationStructure::ObjectInformation;
			return UpdateGeometryResources<GraphicsObjectDescriptor::GeometryDescriptor, ObjectInfo>(
				self, self->m_tlasGeometryResources, rtPass.Tlas().ObjectCount(),
				[&](size_t index) -> const ObjectInfo& { return rtPass.Tlas().ObjectInfo(index); },
				[&](const ObjectInfo& info) -> const GraphicsObjectDescriptor* { return info.graphicsObject; },
				[&](const ObjectInfo& info) -> const GraphicsObjectDescriptor::ViewportData* { return info.viewportData; },
				rtPass);
		}

		inline static bool UpdateObjectDataBuffer(SceneObjectData* self, std::vector<Reference<const Object>>& resourceList) {
			auto fail = [&](const auto... message) {
				ClearResources(self);
				self->m_context->Log()->Error("RayTracedRenderer::Tools::SceneObjectData::UpdateObjectDataBuffer - ", message...);
				return false;
			};

			// Calculate buffer element count for storing all the rasterized geometry information:
			size_t rasterizedGeometrySize = self->m_rasterizedGeometryResources.size();
			for (size_t i = 0u; i < self->m_rasterizedGeometryResources.size(); i++)
				rasterizedGeometrySize = Math::Max(rasterizedGeometrySize, size_t(self->m_rasterizedGeometryResources[i].indirectObjectIndex) + 1u);

			// TLAS geometry size:
			const size_t tlasGeometrySize = self->m_tlasGeometryResources.size();

			// Make sure the buffer is large enough to store both raster and tlas content information:
			size_t bufferSize = 1u;
			while (bufferSize < (rasterizedGeometrySize + tlasGeometrySize))
				bufferSize <<= 1u;

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
				// Map data:
				PerObjectData* objectData = objectDataBuffer.Map();
				if (objectData == nullptr)
					return fail("Failed to map per-object data! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				// Raster part may contain some blans, so we pre-fill it with zeroes for safety:
				for (size_t i = 0u; i < rasterizedGeometrySize; i++)
					objectData[i] = {};

				// Botn raster and TLAS geometries share some of the data within PerObjectData:
				auto copyCommonSettings = [&](PerObjectData& data, const auto& resource) {
					getBufferDeviceAddress(data.materialSettingsBufferId, resource.materialSettingsBuffer, 0u);
					data.materialId = resource.materialId;

					data.flags = resource.flags;
				};

				// Fill rasterized geometry:
				for (size_t i = 0u; i < self->m_rasterizedGeometryResources.size(); i++) {
					const auto& resource = self->m_rasterizedGeometryResources[i];
					assert(resource.indirectObjectIndex < rasterizedGeometrySize);
					PerObjectData& data = objectData[resource.indirectObjectIndex];
					
					data.vertexInput = resource.vertexInput.Get(&resourceList);
					getBufferDeviceAddress(data.indexBufferId, resource.vertexInput.IndexBuffer(), 0u);
					data.firstBlasInstance = 0u;

					copyCommonSettings(data, resource);
				}

				// Fill TLAS geometry:
				for (size_t i = 0u; i < self->m_tlasGeometryResources.size(); i++) {
					const auto& resource = self->m_tlasGeometryResources[i];
					PerObjectData& data = objectData[rasterizedGeometrySize + i];

					data.vertexInput = JM_StandardVertexInput::Get(resource.vertexInput, &resourceList);
					if (resource.vertexInput.indexBuffer.buffer != nullptr) {
						data.indexBufferId = (resource.vertexInput.indexBuffer.buffer->DeviceAddress() + resource.vertexInput.indexBuffer.baseIndexOffset);
						resourceList.push_back(resource.vertexInput.indexBuffer.buffer);
					}
					else data.indexBufferId = 0u;
					data.firstBlasInstance = resource.indirectObjectIndex;

					copyCommonSettings(data, resource);
				}

				// Done:
				objectDataBuffer->Unmap(true);
			}

			// Done:
			self->m_rasterizedGeometrySize = static_cast<uint32_t>(rasterizedGeometrySize);
			return true;
		}
	};

	RayTracedRenderer::Tools::SceneObjectData::SceneObjectData(SharedBindings* sharedBindings)
		: m_context(sharedBindings->tlasViewport->Context())
		, m_perObjectDataBinding(sharedBindings->perObjectDataBinding) {}

	RayTracedRenderer::Tools::SceneObjectData::~SceneObjectData() {}

	bool RayTracedRenderer::Tools::SceneObjectData::Update(
		const GraphicsObjectPipelines::Reader& rasterPipelines,
		const RayTracedPass::State& rtPass, 
		std::vector<Reference<const Object>>& resourceList) {
		
		if (!Helpers::UpdateRasterizedGeometryResources(this, rasterPipelines, rtPass))
			return false;

		if (!Helpers::UpdateRayTracedGeometryResources(this, rtPass))
			return false;

		if (!Helpers::UpdateObjectDataBuffer(this, resourceList))
			return false;

		m_lastRTPipeline = rtPass.Pipeline();
		return true;
	}
}
