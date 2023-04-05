#pragma once
#include "../../../Core/Object.h"
#include "../../../Core/Collections/Stacktor.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class VulkanUnorderedAccessStateManager;
			class VulkanCommandBuffer;
			class VulkanTextureView;
			class VulkanImage;

			/// <summary>
			/// We decided to handle image layout transition "on demand", when a compute pipeline demands it.
			/// To accomplish it, compute binding sets provide BindingSetRWImageInfo with relevant read-write-enabled texture views;
			/// Having said that, SetBindingSetInfo just stores the information, the actual layout transition happens inside 
			/// EnableUnorderedAccess() which is invoked by compute pipeline and undone via DisableUnorderedAccess once the dispatch is done 
			/// and the GENERAL layout is no longer required.
			/// </summary>
			class JIMARA_API VulkanUnorderedAccessStateManager {
			public:
				/// <summary> Destructor </summary>
				~VulkanUnorderedAccessStateManager();

				/// <summary>
				/// If(and when) binding sets contain writable image views,
				/// their layout has to be transitioned to GENERAL,
				/// after which it has to be transitioned back to read-write access.
				/// Since descriptor sets loose access to the command buffer after bind, 
				/// they should provide basic information during the bind call with this structure
				/// </summary>
				struct BindingSetRWImageInfo {
					/// <summary> Binding set id </summary>
					uint32_t bindingSetIndex = 0u;

					/// <summary> 
					/// Images that should be transitioned to GENERAL layout while this descriptor set is bound 
					/// (ei before there's another call to SetBindingSetInfo or CleanBindingSetInfos)
					/// </summary>
					VulkanTextureView** rwImages = nullptr;

					/// <summary> Number of entries in rwImages array </summary>
					size_t rwImageCount = 0u;
				};

				/// <summary>
				/// Sets binding set infos (invoked by binding sets)
				/// </summary>
				/// <param name="info"> Information about binding set's RW indices </param>
				void SetBindingSetInfo(const BindingSetRWImageInfo& info);

				/// <summary>
				/// Removes all information about bound binding sets 
				/// (does not invoke DisableUnorderedAccess; that has to be done separately)
				/// </summary>
				void ClearBindingSetInfos();

				/// <summary>
				/// Enables unordered access views from first bindingSetCount descriptors (Transitions layouts)
				/// <para/> Note: DisableUnorderedAccess() HAS TO BE invoked after pipeline execution to avoid issues in subsequent pipelines!
				/// </summary>
				/// <param name="bindingSetCount"> Number of binding sets to take into consideration </param>
				void EnableUnorderedAccess(uint32_t bindingSetCount);

				/// <summary>
				/// Disables unordered access to the resources previously enabled via EnableUnorderedAccess() call
				/// </summary>
				void DisableUnorderedAccess();

			private:
				// Owner command buffer
				VulkanCommandBuffer* const m_commandBuffer;

				// UAV information from the active binding sets
				using BoundSetRWImageInfo = Stacktor<Reference<VulkanTextureView>, 4u>;
				Stacktor<BoundSetRWImageInfo, 4u> m_boundSetInfos;

				// Transitioned layouts from the last EnableUnorderedAccess() call
				struct TransitionedLayoutInfo {
					Reference<VulkanImage> image;
					uint32_t baseMipLevel = 0u;
					uint32_t mipLevelCount = 0u;
					uint32_t baseArrayLayer = 0u;
					uint32_t arrayLayerCount = 0u;
				};
				Stacktor<TransitionedLayoutInfo, 4u> m_activeUnorderedAccess;

				// No copy/move:
				VulkanUnorderedAccessStateManager(const VulkanUnorderedAccessStateManager&) = delete;
				VulkanUnorderedAccessStateManager& operator=(const VulkanUnorderedAccessStateManager&) = delete;
				VulkanUnorderedAccessStateManager(VulkanUnorderedAccessStateManager&&) = delete;
				VulkanUnorderedAccessStateManager& operator=(VulkanUnorderedAccessStateManager&&) = delete;

				// Only VulkanCommandBuffer can invoke the constructor
				friend class VulkanCommandBuffer;
				VulkanUnorderedAccessStateManager(VulkanCommandBuffer* commandBuffer);
			};
		}
	}
}
#include "../VulkanDevice.h"
