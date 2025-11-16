#include "RayTracedRenderer_Tools.h"


namespace Jimara {
	RayTracedRenderer::Tools::SharedBindings::SharedBindings(
		const RayTracedRenderer* renderSettings,
		SceneLightGrid* sceneLightGrid,
		LightDataBuffer* sceneLightDataBuffer,
		LightTypeIdBuffer* sceneLightTypeIdBuffer,
		AccelerationStructureViewportDesc* viewport,
		Graphics::BindingPool* pool,
		const Graphics::ResourceBinding<Graphics::Buffer>* viewportData)
		: settings(renderSettings)
		, bindlessBuffers(Object::Instantiate<Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::ArrayBuffer>::Instance>>(
			viewport->Context()->Graphics()->Bindless().BufferBinding()))
		, bindlessSamplers(Object::Instantiate<Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::TextureSampler>::Instance>>(
			viewport->Context()->Graphics()->Bindless().SamplerBinding()))
		, lightGrid(sceneLightGrid)
		, lightDataBuffer(sceneLightDataBuffer)
		, lightTypeIdBuffer(sceneLightTypeIdBuffer)
		, tlasViewport(viewport)
		, bindingPool(pool)
		, viewportBuffer(viewportData->BoundObject())
		, viewportBinding(viewportData) {
		assert(settings != nullptr);
		assert(lightGrid != nullptr);
		assert(lightDataBuffer != nullptr);
		assert(lightTypeIdBuffer != nullptr);
		assert(tlasViewport != nullptr);
		assert(viewportBuffer != nullptr);
		assert(viewportBinding != nullptr);
	}

	Reference<RayTracedRenderer::Tools::SharedBindings> RayTracedRenderer::Tools::SharedBindings::Create(
		const RayTracedRenderer* settings, AccelerationStructureViewportDesc* viewport) {
		assert(settings != nullptr);
		auto fail = [&](const auto... message) {
			viewport->Context()->Log()->Error("RayTracedRenderer::Tools::SharedBindings::Create - ", message...);
			return nullptr;
		};

		const Reference<SceneLightGrid> lightGrid = SceneLightGrid::GetFor(viewport->BaseViewport());
		if (lightGrid == nullptr)
			return fail("Failed to get scene light grid pool! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		const Reference<LightDataBuffer> lightDataBuffer = LightDataBuffer::Instance(viewport->BaseViewport());
		if (lightDataBuffer == nullptr)
			return fail("Failed to get light data buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		
		const Reference<LightTypeIdBuffer> lightTypeIdBuffer = LightTypeIdBuffer::Instance(viewport->BaseViewport());
		if (lightTypeIdBuffer == nullptr)
			return fail("Failed to get light type id buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		
		const Reference<Graphics::BindingPool> bindingPool = viewport->Context()->Graphics()->Device()->CreateBindingPool(
			viewport->Context()->Graphics()->Configuration().MaxInFlightCommandBufferCount());
		if (bindingPool == nullptr)
			return fail("Failed to create binding pool! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		const Graphics::BufferReference<ViewportBuffer> viewportBuffer = viewport->Context()->Graphics()->Device()->CreateConstantBuffer<ViewportBuffer>();
		if (viewportBuffer == nullptr)
			return fail("Could not allocate viewport buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		const Reference<const Graphics::ResourceBinding<Graphics::Buffer>> viewportBinding = 
			Object::Instantiate<Graphics::ResourceBinding<Graphics::Buffer>>(viewportBuffer);
		if (viewportBinding == nullptr || viewportBinding->BoundObject() != viewportBuffer)
			return fail("Could not allocate viewport buffer binding! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		Reference<SharedBindings> bindings = new SharedBindings(settings, lightGrid, lightDataBuffer, lightTypeIdBuffer, viewport, bindingPool, viewportBinding);
		bindings->ReleaseRef();
		return bindings;
	}

	void RayTracedRenderer::Tools::SharedBindings::Update(uint32_t rasterizedGeometrySize) {
		// View/projection:
		{
			viewportBufferData.view = tlasViewport->BaseViewport()->ViewMatrix();
			viewportBufferData.projection = tlasViewport->BaseViewport()->ProjectionMatrix();
		}

		// Inverse view/projection:
		{
			auto safeInvert = [](const Matrix4& matrix) {
				const Matrix4 inverse = Math::Inverse(matrix);
				auto hasNanOrInf = [](const Vector4& column) {
					auto check = [](float f) { return std::isnan(f) || std::isinf(f); };
					return (check(column.x) || check(column.y) || check(column.z) || check(column.w));
				};
				if (hasNanOrInf(inverse[0u]) ||
					hasNanOrInf(inverse[1u]) ||
					hasNanOrInf(inverse[2u]) ||
					hasNanOrInf(inverse[3u])) return Math::Identity();
				return inverse;
			};
			viewportBufferData.viewPose = safeInvert(viewportBufferData.view);
			viewportBufferData.inverseProjection = safeInvert(viewportBufferData.projection);
		}

		// Geometry counts:
		viewportBufferData.rasterizedGeometrySize = rasterizedGeometrySize;

		// Flags:
		viewportBufferData.renderFlags = settings->Flags();
		
		// Range:
		{
			viewportBufferData.accelerationStructureRange = settings->AccelerationStructureRange();

			auto cameraClipToLocalSpace = [&](float x, float y, float z) -> Vector3 {
				Vector4 clipPos = viewportBufferData.inverseProjection * Vector4(x, y, z, 1.0f);
				return (clipPos / clipPos.w);
			};

			if ((viewportBufferData.renderFlags & RendererFlags::SCALE_ACCELERATION_STRUCTURE_RANGE_BY_FAR_PLANE) != RendererFlags::NONE) {
				const float baseFarPlane = cameraClipToLocalSpace(0.0f, 0.0f, 1.0f).z;
				viewportBufferData.accelerationStructureRange *= baseFarPlane;
			}
		}

		// Trace-depth:
		viewportBufferData.maxTraceDepth = settings->MaxTraceDepth();

		// TLAS-Viewport:
		tlasViewport->Update(viewportBufferData.accelerationStructureRange);

		// Light data:
		{
			lightDataBinding->BoundObject() = lightDataBuffer->Buffer();
			lightTypeIdBinding->BoundObject() = lightTypeIdBuffer->Buffer();
		}

		// Viewport data buffer:
		{	
			viewportBuffer.Map() = viewportBufferData;
			viewportBuffer->Unmap(true);
		}

		// Eye-pos:
		eyePosition = tlasViewport->BaseViewport()->EyePosition();
	}

	Reference<Graphics::BindingSet> RayTracedRenderer::Tools::SharedBindings::CreateBindingSet(
		Graphics::Pipeline* pipeline, size_t bindingSetId,
		const Graphics::BindingSet::BindingSearchFunctions& additionalSearchFunctions)const {

		const Graphics::BindingSet::BindingSearchFunctions lightGridBindings = lightGrid->BindingDescriptor();

		Graphics::BindingSet::Descriptor desc = {};
		desc.pipeline = pipeline;
		desc.bindingSetId = bindingSetId;
		
		auto findBindlessBuffers = [&](const Graphics::BindingSet::BindingDescriptor&) {
			return bindlessBuffers;
		};
		desc.find.bindlessStructuredBuffers = &findBindlessBuffers;

		auto findBindlessSamplers = [&](const Graphics::BindingSet::BindingDescriptor&) {
			return bindlessSamplers;
		};
		desc.find.bindlessTextureSamplers = &findBindlessSamplers;

		auto findSturucturedBuffers = [&](const Graphics::BindingSet::BindingDescriptor& binding) 
			-> Reference<const Graphics::ResourceBinding<Graphics::ArrayBuffer>> {
			if (binding.name == LIGHT_DATA_BUFFER_NAME)
				return lightDataBinding;
			else if (binding.name == LIGHT_TYPE_IDS_BUFFER_NAME)
				return lightTypeIdBinding;
			else if (binding.name == SCENE_OBJECT_DATA_BUFFER_NAME)
				return perObjectDataBinding;
			Reference<const Graphics::ResourceBinding<Graphics::ArrayBuffer>> bnd = lightGridBindings.structuredBuffer(binding);
			if (bnd != nullptr)
				return bnd;
			return additionalSearchFunctions.structuredBuffer(binding);
		};
		desc.find.structuredBuffer = &findSturucturedBuffers;

		auto findCbuffers = [&](const Graphics::BindingSet::BindingDescriptor& binding) 
			-> Reference<const Graphics::ResourceBinding<Graphics::Buffer>> {
			if (binding.name == VIEWPORT_BUFFER_NAME)
				return viewportBinding;
			Reference<const Graphics::ResourceBinding<Graphics::Buffer>> bnd = lightGridBindings.constantBuffer(binding);
			if (bnd != nullptr)
				return bnd;
			return additionalSearchFunctions.constantBuffer(binding);
		};
		desc.find.constantBuffer = &findCbuffers;

		desc.find.textureSampler = additionalSearchFunctions.textureSampler;

		desc.find.textureView = additionalSearchFunctions.textureView;

		desc.find.accelerationStructure = additionalSearchFunctions.accelerationStructure;
		
		const Reference<Graphics::BindingSet> set = bindingPool->AllocateBindingSet(desc);
		if (set == nullptr) {
			tlasViewport->Context()->Log()->Error(
				"RayTracedRenderer::Tools::SharedBindings::CreateBindingSet - ",
				"Failed to allocate binding set! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		}
		return set;
	}
}
