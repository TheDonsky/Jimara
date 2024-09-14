#include "SimpleComputeKernel.h"


namespace Jimara {
	struct SimpleComputeKernel::Helpers {
		class Implementation : public virtual SimpleComputeKernel {
		private:
			const Reference<Graphics::GraphicsDevice> m_device;
			const Reference<Graphics::ComputePipeline> m_pipeline;
			const Stacktor<Reference<Graphics::BindingSet>, 1u> m_bindingSets;

			friend class SimpleComputeKernel;

		public:
			inline Implementation(
				Graphics::GraphicsDevice* device,
				Graphics::ComputePipeline* pipeline,
				const Stacktor<Reference<Graphics::BindingSet>, 1u>& bindingSets)
				: m_device(device)
				, m_pipeline(pipeline)
				, m_bindingSets(bindingSets) {
				assert(m_device != nullptr);
				assert(m_pipeline != nullptr);
				assert(m_bindingSets.Size() == m_pipeline->BindingSetCount());
			}

			inline virtual ~Implementation() {}
		};
	};

	Reference<SimpleComputeKernel> SimpleComputeKernel::Create(
		Graphics::GraphicsDevice* device,
		ShaderLibrary* shaderLibrary,
		Graphics::BindingPool* bindingPool,
		const Graphics::ShaderClass* computeShader,
		const Graphics::BindingSet::BindingSearchFunctions& bindings) {
		if (device == nullptr) return nullptr;
		auto fail = [&](const auto&... message) {
			device->Log()->Error("SimpleComputeKernel::Create - ", message...);
			return nullptr;
		};

		if (bindingPool == nullptr)
			return fail("Binding pool missing! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		if (computeShader == nullptr)
			return fail("Compute shader address not provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		if (shaderLibrary == nullptr)
			return fail("Shader library not provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		
		const std::string shaderPath = std::string(computeShader->ShaderPath()) + ".comp";
		const Reference<Graphics::SPIRV_Binary> shader = shaderLibrary->LoadShader(shaderPath);
		if (shader == nullptr)
			return fail("Failed to get/load shader module for '", computeShader->ShaderPath(), "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		const Reference<Graphics::ComputePipeline> pipeline = device->GetComputePipeline(shader);
		if (pipeline == nullptr)
			return fail("Failed to get/create compute pipeline for '", computeShader->ShaderPath(), "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		Graphics::BindingSet::Descriptor setDesc = {};
		setDesc.pipeline = pipeline;
		setDesc.find = bindings;
		Stacktor<Reference<Graphics::BindingSet>, 1u> bindingSets;
		for (size_t i = 0u; i < pipeline->BindingSetCount(); i++) {
			setDesc.bindingSetId = i;
			const Reference<Graphics::BindingSet> bindingSet = bindingPool->AllocateBindingSet(setDesc);
			if (bindingSet == nullptr)
				return fail("Failed to allocate binding set! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			else bindingSets.Push(bindingSet);
		}

		return Object::Instantiate<Helpers::Implementation>(device, pipeline, bindingSets);
	}

	Reference<SimpleComputeKernel> SimpleComputeKernel::Create(
		Graphics::GraphicsDevice* device,
		ShaderLibrary* shaderLibrary,
		size_t maxInFlightCommandBuffers,
		const Graphics::ShaderClass* computeShader,
		const Graphics::BindingSet::BindingSearchFunctions& bindings) {
		if (device == nullptr) return nullptr;
		auto fail = [&](const auto&... message) {
			device->Log()->Error("SimpleComputeKernel::Create - ", message...);
			return nullptr;
		};
		const Reference<Graphics::BindingPool> bindingPool = device->CreateBindingPool(maxInFlightCommandBuffers);
		if (bindingPool == nullptr)
			return fail("Failed to create binding pool! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		return Create(device, shaderLibrary, bindingPool, computeShader, bindings);
	}

	SimpleComputeKernel::SimpleComputeKernel() {}

	SimpleComputeKernel::~SimpleComputeKernel() {}

	void SimpleComputeKernel::Dispatch(const Graphics::InFlightBufferInfo& commandBuffer, const Size3& workgroupCount)const {
		const Helpers::Implementation* self = dynamic_cast<const Helpers::Implementation*>(this);
		for (size_t i = 0u; i < self->m_bindingSets.Size(); i++) {
			Graphics::BindingSet* set = self->m_bindingSets[i];
			set->Update(commandBuffer);
			set->Bind(commandBuffer);
		}
		self->m_pipeline->Dispatch(commandBuffer, workgroupCount);
	}
}
