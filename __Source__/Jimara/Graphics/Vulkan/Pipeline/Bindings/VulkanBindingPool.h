#pragma once
#include "../Pipelines/VulkanPipeline.h"
#include "../../Memory/Buffers/VulkanConstantBuffer.h"
#include <set>


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class VulkanBindingSet;

			/// <summary>
			/// BindingPool implementation for Vulkan API
			/// </summary>
			class JIMARA_API VulkanBindingPool : public virtual BindingPool {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> Graphics device </param>
				/// <param name="inFlightCommandBufferCount"> Maximal number of allowed in-flight command buffers </param>
				VulkanBindingPool(VulkanDevice* device, size_t inFlightCommandBufferCount);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanBindingPool();

				/// <summary>
				/// Creates/Allocated new binding set instance
				/// </summary>
				/// <param name="descriptor"> Binding set descriptor </param>
				/// <returns> New instance of a binding set </returns>
				virtual Reference<BindingSet> AllocateBindingSet(const BindingSet::Descriptor& descriptor) override;

				/// <summary>
				/// Equivalent of invoking BindingSet::Update() on all binding sets allocated from this pool.
				/// <para/> Notes: 
				///		<para/> 0. UpdateAllBindingSets is usually much faster than invoking BindingSet::Update() on all sets;
				///		<para/> 1. Using muptiple threads to invoke BindingSet::Update() will not give better performance, 
				///			since BindingPool objects are expected to be internally synchronized.
				/// </summary>
				/// <param name="inFlightCommandBufferIndex"> Index of an in-flight command buffer for duture binding </param>
				virtual void UpdateAllBindingSets(size_t inFlightCommandBufferIndex) override;


			private:
				// Graphics device
				const Reference<VulkanDevice> m_device;

				// Maximal number of allowed in-flight command buffers
				const size_t m_inFlightCommandBufferCount;

				// Lock for updates
				std::mutex m_poolDataLock;

				// Reference to the latest VkDescriptorPool bucket
				Reference<Object> m_bindingBucket;
				
				// Collection of allocated sets
				struct {
					std::set<VulkanBindingSet*> sets;
					std::vector<VulkanBindingSet*> sortedSets;
				} m_allocatedSets;

				// Private stuff
				struct Helpers;

				// VulkanBindingSet needs access to internals for locking and managing m_allocatedSets
				friend class VulkanBindingSet;
			};

			/// <summary>
			/// BindingSet implementation for Vulkan API
			/// </summary>
			class JIMARA_API VulkanBindingSet : public virtual BindingSet {
			public:
				/// <summary> Virtual destructor </summary>
				virtual ~VulkanBindingSet();

				/// <summary>
				/// Stores currently bound resources from the user-provided resource bindings and updates underlying API objects.
				/// <para/> Notes: 
				///		<para/> 0. General usage would be as follows:
				///			<para/> InFlightBufferInfo info = ...
				///			<para/>	bindingSet->Update(info.inFlightBufferId);
				///			<para/> bindingSet->Bind(info);
				///		<para/> 1. If there are many binding sets from the same descriptor pool, 
				///			it will be more optimal to use BindingPool::UpdateAllBindingSets() instead.
				/// </summary>
				/// <param name="inFlightCommandBufferIndex"> Index of an in-flight command buffer for duture binding </param>
				virtual void Update(size_t inFlightCommandBufferIndex) override;

				/// <summary>
				/// Binds descriptor set for future pipeline executions.
				/// <para/> Note: All relevant binding sets should be bound using this call before the dispatch or a draw call.
				/// </summary>
				/// <param name="inFlightBuffer"> Command buffer and in-flight index </param>
				virtual void Bind(InFlightBufferInfo inFlightBuffer) override;

			private:
				// Shorthand for Reference<const ResourceBinding<ResourceType>>
				template<typename ResourceType>
				using Binding = Reference<const ResourceBinding<ResourceType>>;

				// Resource Binding and binding slot index
				template<typename ResourceType>
				struct BindingInfo {
					Binding<ResourceType> binding;
					uint32_t bindingIndex = 0u;
				};

				// List of bindings
				template<typename ResourceType>
				using Bindings = Stacktor<BindingInfo<ResourceType>, 4u>;

				// Bindings per type
				struct SetBindings {
					Bindings<Buffer> constantBuffers;
					Bindings<ArrayBuffer> structuredBuffers;
					Bindings<TextureSampler> textureSamplers;
					Bindings<TextureView> textureViews;
					Binding<BindlessSet<ArrayBuffer>::Instance> bindlessStructuredBuffers;
					Binding<BindlessSet<TextureSampler>::Instance> bindlessTextureSamplers;
				};

				// Vulkan descriptor sets
				using DescriptorSets = Stacktor<VkDescriptorSet, 4u>;

				// Pipeline used for the binding set allocation
				const Reference<const VulkanPipeline> m_pipeline;

				// VulkanBindingPool this binding set was allocated from
				const Reference<VulkanBindingPool> m_bindingPool;

				// Reference to the VkDescriptorPool bucket this objects was allocated from
				const Reference<Object> m_bindingBucket;

				// Bindings per type
				const SetBindings m_bindings;

				// Vulkan descriptor sets
				const DescriptorSets m_descriptors;

				// Binding set index within the shader(s)
				const uint32_t m_bindingSetIndex;

				// Pipeline stages, this set is used in
				const PipelineStageMask m_pipelineStageMask;

				// Total number of bindings
				size_t m_setBindingCount;

				// Last used constant buffers
				Stacktor<Reference<VulkanPipelineConstantBuffer>, 4u> m_cbufferInstances;

				// Bound objects (sequential sublists of m_setBindingCount elements represent active bindings per in-flight buffer)
				Stacktor<Reference<Object>, 16u> m_boundObjects;

				// Constructor is private
				VulkanBindingSet(
					VulkanBindingPool* bindingPool, const VulkanPipeline* pipeline, Object* bindingBucket,
					SetBindings&& bindings, DescriptorSets&& descriptors,
					uint32_t bindingSetIndex, PipelineStageMask pipelineStageMask);

				// Binding pool can use the constructor, actually... Because it's such a good friend
				friend class VulkanBindingPool;
			};
		}
	}
}
