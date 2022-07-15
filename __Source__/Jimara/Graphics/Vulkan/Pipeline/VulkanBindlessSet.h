#pragma once
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			template<typename DataType> class VulkanBindlessBinding;
			template<typename DataType> class VulkanBindlessInstance;
			template<typename DataType> class VulkanBindlessSet;
		}
	}
}
#include "../VulkanDevice.h"
#include "../Memory/Buffers/VulkanArrayBuffer.h"
#include "../Memory/Buffers/VulkanConstantBuffer.h"
#include "../Memory/Textures/VulkanTextureSampler.h"
#include "../../Pipeline/BindlessSet.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// Bindless set binding implementation for Vulkan API
			/// </summary>
			/// <typeparam name="DataType"> Bound resource type </typeparam>
			template<typename DataType>
			class JIMARA_API VulkanBindlessBinding : public virtual BindlessSet<DataType>::Binding {
			public:
				/// <summary> Object, associated with the index </summary>
				inline virtual DataType* BoundObject()const override { return m_value; }


			protected:
				/// <summary>
				/// Returns VulkanBindlessBinding to the freelist of the owner VulkanBindlessSet
				/// </summary>
				inline virtual void OnOutOfScope()const override;

			private:
				// Object, associated with the index
				mutable Reference<DataType> m_value;

				// Bindless set, this descriptor is from
				mutable Reference<VulkanBindlessSet<DataType>> m_owner;

				// VulkanBindlessSet and VulkanBindlessInstance are friendly
				friend class VulkanBindlessSet<DataType>;
				friend class VulkanBindlessInstance<DataType>;

				// Only VulkanBindlessSet is allowed to invoke the constructor
				inline VulkanBindlessBinding(uint32_t index) : VulkanBindlessSet<DataType>::Binding(index) {}
			};

			/// <summary>
			/// BindlessSet instance implementation for Vulkan API
			/// </summary>
			/// <typeparam name="DataType"> Bound resource type </typeparam>
			template<typename DataType>
			class JIMARA_API VulkanBindlessInstance : public virtual BindlessSet<DataType>::Instance {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="owner"> VulkanBindlessSet, this instance takes records from </param>
				/// <param name="maxInFlightCommandBuffers"> Maximal number of in-flight command buffers that can simultanuously use this instance </param>
				inline VulkanBindlessInstance(VulkanBindlessSet<DataType>* owner, size_t maxInFlightCommandBuffers);

				/// <summary> Virtual destructor </summary>
				inline virtual ~VulkanBindlessInstance();

				/// <summary>
				/// Gets descriptor set for given in-flight command buffer id
				/// </summary>
				/// <param name="inFlightBufferId"> in-flight command buffer id </param>
				/// <returns> Vulkan descriptor set </returns>
				inline VkDescriptorSet GetDescriptorSet(size_t inFlightBufferId);

			private:
				// VulkanBindlessSet, this instance takes records from
				const Reference<VulkanBindlessSet<DataType>> m_owner;

				// Descriptor pool and pipeline layout
				VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
				VkDescriptorSetLayout m_setLayout = VK_NULL_HANDLE;

				// Cached binding value
				struct CachedBinding {
					Reference<DataType> value;
					bool dirty = true;
				};

				// Data per command buffer
				struct CommandBufferData {
					std::mutex updateLock;
					std::vector<CachedBinding> cachedBindings;
					std::vector<uint32_t> dirtyIndices;
					std::atomic<bool> dirty = true;
					VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

					inline CommandBufferData() {}
					inline CommandBufferData(const CommandBufferData& other)
						: cachedBindings(other.cachedBindings), dirtyIndices(other.dirtyIndices), dirty(other.dirty.load()) {}
					inline CommandBufferData(CommandBufferData&& other)
						: cachedBindings(std::move(other.cachedBindings)), dirtyIndices(std::move(other.dirtyIndices)), dirty(other.dirty.load()) {}
				};
				std::vector<CommandBufferData> m_bufferData;

				// Invoked each time a binding gets allocated or goes out of scope
				inline void IndexDirty(uint32_t index);

				// Some private helpers reside here
				struct Helpers;
			};

			/// <summary>
			/// BindlessSet implementation for Vulkan backend
			/// </summary>
			/// <typeparam name="DataType"> Bound resource type </typeparam>
			template<typename DataType>
			class JIMARA_API VulkanBindlessSet : public virtual BindlessSet<DataType> {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> Vulkan device, associated with this binding set </param>
				inline VulkanBindlessSet(VulkanDevice* device);

				/// <summary> Virtual destructor </summary>
				inline virtual ~VulkanBindlessSet();

				/// <summary> Limit on how many bindings a single bindless set can hold before it craps out </summary>
				inline static const constexpr uint32_t MaxBoundObjects() {
					return sizeof(((VulkanBindlessSet*)nullptr)->m_bindings) / sizeof(VulkanBindlessBinding<DataType>);
				}


				/// <summary>
				/// Creates or retrieves bindless "binding" of given object 
				/// <para/> Notes:
				///		<para/> 0. Calls with the same resource as a parameter will return the same object;
				///		<para/> 1. nullptr is a valid binding, although your shader may not be a fan of it;
				///		<para/> 2. Since the index association is technically only alive till the moment Binding goes out of scope,
				///				it's adviced to keep alive Binding objects for the whole duration of the frame they are being used throught.
				/// </summary>
				/// <param name="object"> Object to associate with some index </param>
				/// <returns> Object to array index association token </returns>
				inline virtual Reference<typename BindlessSet<DataType>::Binding> GetBinding(DataType* object) override;

				/// <summary>
				/// Creates an instance of the bindless set that can be shared among pipelines
				/// </summary>
				/// <param name="maxInFlightCommandBuffers"> 
				///		Maximal number of in-flight command buffers that can simultanuously use this instance
				///		(There is no limit on the number of pipelines that use the instance with same in-flight buffer id)
				/// </param>
				/// <returns> Bindless set instance </returns>
				inline virtual Reference<typename BindlessSet<DataType>::Instance> CreateInstance(size_t maxInFlightCommandBuffers) override;


			private:
				// Owner device
				const Reference<VulkanDevice> m_device;

				// 'Empty' binding for null descriptors
				const Reference<DataType> m_emptyBinding;

				// Lock for binding creation/deletion
				std::shared_mutex m_lock;

				// Actual bindings
				char m_bindings[sizeof(VulkanBindlessBinding<DataType>) * (1 << 18)];
				
				// List of unused binding indices
				std::vector<uint32_t> m_freeList;

				// Object to index binding map
				std::unordered_map<DataType*, uint32_t> m_index;

				// Invoked each time new binding gets created or some old one goes out of scope
				EventInstance<uint32_t> m_descriptorDirty;

				// Type casts m_bindings to concrete type pointer
				inline VulkanBindlessBinding<DataType>* Bindings() { return reinterpret_cast<VulkanBindlessBinding<DataType>*>(m_bindings + 0); }

				// VulkanBindlessBinding and VulkanBindlessInstance are friendly
				friend class VulkanBindlessBinding<DataType>;
				friend class VulkanBindlessInstance<DataType>;
			};
		}
	}
}
#include "VulkanBindlessSet.cpp"
