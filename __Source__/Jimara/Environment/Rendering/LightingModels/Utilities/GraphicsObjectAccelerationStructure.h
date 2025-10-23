#pragma once
#include "../../../Scene/Scene.h"
#include "../../SceneObjects/Objects/ViewportGraphicsObjectSet.h"
#include "JM_StandardVertexInputStructure.h"
#include "../../../Layers.h"


namespace Jimara {
	class JIMARA_API GraphicsObjectAccelerationStructure : public virtual Object {
	public:
		enum class JIMARA_API Flags : uint32_t {
			NONE = 0
		};

		struct JIMARA_API Descriptor {
			/// <summary> Set of GraphicsObjectDescriptor-s </summary>
			Reference<GraphicsObjectDescriptor::Set> descriptorSet;

			/// <summary> Renderer frustrum descriptor </summary>
			Reference<const RendererFrustrumDescriptor> frustrumDescriptor;

			/// <summary> GraphicsObjectDescriptor layers for filtering </summary>
			LayerMask layers = LayerMask::All();

			/// <summary> Additional flags for filtering and overrides </summary>
			Flags flags = Flags::NONE;

			/// <summary> Comparator (equals) </summary>
			bool operator==(const Descriptor& other)const;

			/// <summary> Comparator (less) </summary>
			bool operator<(const Descriptor& other)const;
		};

		static Reference<GraphicsObjectAccelerationStructure> GetFor(const Descriptor& desc);

	private:
		struct Helpers;
	};


	/// <summary> Boolean opeartors for the flags. </summary>
	JIMARA_DEFINE_ENUMERATION_BOOLEAN_OPERATIONS(GraphicsObjectAccelerationStructure::Flags);
}


namespace std {
	/// <summary> std::hash override for Jimara::GraphicsObjectPipelines::Descriptor </summary>
	template<>
	struct JIMARA_API hash<Jimara::GraphicsObjectAccelerationStructure::Descriptor> {
		/// <summary> std::hash override for Jimara::GraphicsObjectPipelines::Descriptor </summary>
		size_t operator()(const Jimara::GraphicsObjectAccelerationStructure::Descriptor& descriptor)const;
	};
}

