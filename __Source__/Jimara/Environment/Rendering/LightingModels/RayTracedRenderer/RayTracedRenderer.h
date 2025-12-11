#pragma once
#include "../LightingModel.h"
#include "../../../../Data/ConfigurableResource.h"


namespace Jimara {
	/// <summary> Let the system know the resource type exists </summary>
	JIMARA_REGISTER_TYPE(Jimara::RayTracedRenderer);

#pragma warning(disable: 4250)
	/// <summary>
	/// Ray-traced lighting model
	/// </summary>
	class JIMARA_API RayTracedRenderer
		: public virtual LightingModel
		, public virtual ConfigurableResource {
	public:
		enum class RendererFlags : uint32_t {
			NONE = 0,
			USE_RASTER_VBUFFER = (1 << 0),
			FALLBACK_ON_FIRST_RAY_IF_VBUFFER_EVAL_FAILS = (1 << 1),
			DISCARD_IRRADIANCE_PHOTONS_IF_RAY_DEPTH_THRESHOLD_REACHED = (1 << 2),

			SCALE_ACCELERATION_STRUCTURE_RANGE_BY_FAR_PLANE = (1 << 3),

			DEFAULT = (
				USE_RASTER_VBUFFER |
				FALLBACK_ON_FIRST_RAY_IF_VBUFFER_EVAL_FAILS |
				SCALE_ACCELERATION_STRUCTURE_RANGE_BY_FAR_PLANE)
		};

		static const Object* RendererFlagsEnumAttribute();

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name=""> Ignored </param>
		inline RayTracedRenderer(ConfigurableResource::CreateArgs = {}) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~RayTracedRenderer() {}

		/// <summary>
		/// Creates a ray-traced renderer
		/// </summary>
		/// <param name="viewport"> Render viewport descriptor </param>
		/// <param name="layers"> Rendered layer mask </param>
		/// <param name="flags"> Clear/Resolve flags </param>
		/// <returns> A new instance of a renderer </returns>
		virtual Reference<RenderStack::Renderer> CreateRenderer(const ViewportDescriptor* viewport, LayerMask layers, Graphics::RenderPass::Flags flags)const override;

		/// <summary>
		/// Gives access to sub-serializers/fields
		/// </summary>
		/// <param name="recordElement"> Each sub-serializer should be reported by invoking this callback with serializer & corresonding target as parameters </param>
		virtual void GetFields(Callback<Serialization::SerializedObject> recordElement) override;

		inline RendererFlags Flags()const { return m_flags; }

		inline void SetFlags(RendererFlags flags) { m_flags = flags; }

		inline float AccelerationStructureRange()const { return m_accelerationStructureSize; }

		inline void SetAccelerationStructureRange(float range) { m_accelerationStructureSize = range; }

		inline uint32_t MaxTraceDepth()const { return m_maxTraceDepth; }

		inline void SetMaxTraceDepth(uint32_t depth) { m_maxTraceDepth = depth; }

		inline uint32_t SamplesPerPixel()const { return m_samplesPerPixel; }

		inline void SetSamplesPerPixel(uint32_t samples) { m_samplesPerPixel = Math::Max(samples, 1u); }

	private:
		// Underlying passes and common tools reside in-here (defined in RayTracedRenderer_Tools.h, only used internally)
		struct Tools;

		std::atomic<RendererFlags> m_flags = RendererFlags::DEFAULT;
		std::atomic<float> m_accelerationStructureSize = 1.0;
		std::atomic<uint32_t> m_maxTraceDepth = 4u;
		std::atomic<uint32_t> m_samplesPerPixel = 1u;
	};
#pragma warning(default: 4250)

	// Define booleans for flags:
	JIMARA_DEFINE_ENUMERATION_BOOLEAN_OPERATIONS(RayTracedRenderer::RendererFlags);

	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<RayTracedRenderer>(const Callback<TypeId>& report) {
		report(TypeId::Of<LightingModel>());
		report(TypeId::Of<ConfigurableResource>());
	}
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<RayTracedRenderer>(const Callback<const Object*>& report);
}
