#include "RayTracedRenderer_Tools.h"


namespace Jimara {
	RayTracedRenderer::Tools::SharedBindings::SharedBindings(
		SceneLightGrid* sceneLightGrid,
		LightDataBuffer* sceneLightDataBuffer,
		LightTypeIdBuffer* sceneLightTypeIdBuffer,
		const ViewportDescriptor* viewportDesc,
		Graphics::BindingPool* pool,
		const Graphics::ResourceBinding<Graphics::Buffer>* viewportData)
		: bindlessBuffers(Object::Instantiate<Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::ArrayBuffer>::Instance>>(
			viewportDesc->Context()->Graphics()->Bindless().BufferBinding()))
		, bindlessSamplers(Object::Instantiate<Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::TextureSampler>::Instance>>(
			viewportDesc->Context()->Graphics()->Bindless().SamplerBinding()))
		, lightGrid(sceneLightGrid)
		, lightDataBuffer(sceneLightDataBuffer)
		, lightTypeIdBuffer(sceneLightTypeIdBuffer)
		, viewport(viewportDesc)
		, bindingPool(pool)
		, viewportBuffer(viewportData->BoundObject())
		, viewportBinding(viewportData) {
		assert(lightGrid != nullptr);
		assert(lightDataBuffer != nullptr);
		assert(sceneLightTypeIdBuffer != nullptr);
		assert(viewport != nullptr);
		assert(viewportBuffer != nullptr);
		assert(viewportBinding != nullptr);
	}

	Reference<RayTracedRenderer::Tools::SharedBindings> RayTracedRenderer::Tools::SharedBindings::Create(const ViewportDescriptor* viewport) {
		auto fail = [&](const auto... message) {
			viewport->Context()->Log()->Error("RayTracedRenderer::Tools::SharedBindings::Create - ", message...);
			return nullptr;
		};

		const Reference<SceneLightGrid> lightGrid = SceneLightGrid::GetFor(viewport);
		if (lightGrid == nullptr)
			return fail("Failed to get scene light grid pool! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		const Reference<LightDataBuffer> lightDataBuffer = LightDataBuffer::Instance(viewport);
		if (lightDataBuffer == nullptr)
			return fail("Failed to get light data buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		
		const Reference<LightTypeIdBuffer> lightTypeIdBuffer = LightTypeIdBuffer::Instance(viewport);
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

		Reference<SharedBindings> bindings = new SharedBindings(lightGrid, lightDataBuffer, lightTypeIdBuffer, viewport, bindingPool, viewportBinding);
		bindings->ReleaseRef();
		return bindings;
	}

	void RayTracedRenderer::Tools::SharedBindings::Update(uint32_t rasterizedGeometrySize) {
		lightDataBinding->BoundObject() = lightDataBuffer->Buffer();
		lightTypeIdBinding->BoundObject() = lightTypeIdBuffer->Buffer();

		viewportBufferData.view = viewport->ViewMatrix();
		viewportBufferData.projection = viewport->ProjectionMatrix();
		viewportBufferData.viewPose = Math::Inverse(viewportBufferData.view);
		viewportBufferData.inverseProjection = Math::Inverse(viewportBufferData.projection);
		viewportBufferData.rasterizedGeometrySize = rasterizedGeometrySize;

		eyePosition = viewport->EyePosition();
		viewportBuffer.Map() = viewportBufferData;
		viewportBuffer->Unmap(true);
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
			viewport->Context()->Log()->Error(
				"RayTracedRenderer::Tools::SharedBindings::CreateBindingSet - ",
				"Failed to allocate binding set! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		}
		return set;
	}
}
