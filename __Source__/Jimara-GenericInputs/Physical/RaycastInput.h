#pragma once
#include "../Base/VectorInput.h"
#include <Jimara/Components/Physics/Collider.h>


namespace Jimara {
	/// <summary> Let the system know about the type </summary>
	JIMARA_REGISTER_TYPE(Jimara::RaycastInput);

	/// <summary>
	/// Base class of RaycastInput; Not intended to be used by end-user;
	/// <para/> Intent of this one is to expose InputProvider<RaycastHit> interface through RaycastInput and nothing more..
	/// </summary>
	class JIMARA_GENERIC_INPUTS_API RaycastInput_Base_RaycastHitInput : public virtual InputProvider<RaycastHit> {
	public:
		/// <summary>
		/// Evaluates RaycastHit input
		/// </summary>
		/// <returns> dynamic_cast&lt;RaycastInput&gt;(this)->EvaluateRaycastHitResult() </returns>
		virtual std::optional<RaycastHit> GetInput()final override;

	private:
		// Constructor can only be accessed through RaycastInput
		inline RaycastInput_Base_RaycastHitInput() {}
		friend class RaycastInput;
	};


	/// <summary>
	/// An input provider that performs a raycast/sweep and returns RaycastHit and/or hit-distance
	/// <para/> Floating-point-type Vector-input evaluates hit distance (nullopt if there's no hit);
	/// <para/> RaycastHit input provider interface evaluates the hit itself.
	/// </summary>
	class JIMARA_GENERIC_INPUTS_API RaycastInput
		: public virtual VectorInput::ComponentFrom<float>
		, public RaycastInput_Base_RaycastHitInput {
	public:
		/// <summary> Type of the raycast or a sweep </summary>
		enum class QueryType : uint8_t {
			/// <summary> Query will be a Raycast </summary>
			RAY = 0,

			/// <summary> Query will be a Sweep with a sphere as the shape </summary>
			SPHERE = 1,

			/// <summary> Query will be a Sweep with a capsule as the shape </summary>
			CAPSULE = 2,

			/// <summary> Query will be a Sweep with a box as the shape </summary>
			BOX = 3,

			/// <summary> Basically disables the query </summary>
			NONE = 4
		};

		/// <summary> Enum-attribute for query-type </summary>
		static const Object* QueryTypeEnumerationAttribute();

		/// <summary> Flags and options for the query </summary>
		enum class Flags : uint16_t {
			/// <summary> Empty bitmask </summary>
			NONE = 0u,

			/// <summary> Will update input even when the RaycastInput component is disabled; by default, disabling the component will also disable the input </summary>
			QUERY_WHEN_DISABLED = (1 << 0),

			/// <summary> Excludes dynamic bodies from the query </summary>
			EXCLUDE_DYNAMIC_BODIES = (1 << 1),

			/// <summary> Excludes static bodies from the query </summary>
			EXCLUDE_STATIC_BODIES = (1 << 2),

			/// <summary> If set, this flag will cause the query to originate in the world-space instead of transform's local space </summary>
			QUERY_ORIGIN_WORLD_SPACE = (1 << 3),

			/// <summary> If set, this flag will cause the query direction will be in the world-space instead of transform's local space </summary>
			QUERY_DIRECTION_WORLD_SPACE = (1 << 4),

			/// <summary> If this flag is set, max distance will be scaled by lossy scale of the transform </summary>
			SCALE_MAX_DISTANCE_BY_LOSSY_SCALE = (1 << 5),

			/// <summary> If this flag is set, sweep shape will be scaled by lossy scale of the transform for SPHERE CAPSULE and BOX QueryType </summary>
			SCALE_SWEEP_SHAPE_BY_LOSSY_SCALE = (1 << 6),

			/// <summary> If set, sweep-shape will not be rotated with the transform for SPHERE CAPSULE and BOX QueryType </summary>
			DO_NOT_ROTATE_SWEEP_SHAPE = (1 << 7),

			/// <summary> 
			/// If set, the input provider will perform a new raycast/sweep each time the input is queried 
			/// instead of caching the 'current frame result' 
			/// </summary>
			DISABLE_FRAME_CACHING = (1 << 8)
		};

		/// <summary> Enum-attribute for Flags </summary>
		static const Object* FlagOptionsEnumerationAttribute();


		/// <summary> Filter input for filtering-out Colliders </summary>
		using ColliderFilterInput = InputProvider<bool, Collider*>;

		/// <summary> Filter input for filtering-out candidate RaycastHit results </summary>
		using RayHitFilterInput = InputProvider<bool, const RaycastHit&>;


		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		RaycastInput(Component* parent, const std::string_view& name = "RaycastInput");

		/// <summary> Type of the raycast or a sweep </summary>
		inline QueryType QueryMode()const { return m_queryType; }

		/// <summary>
		/// Sets ray/sweep type
		/// </summary>
		/// <param name="mode"> Type of the raycast or a sweep </param>
		inline void SetQueryMode(const QueryType mode) { m_queryType = Math::Min(mode, QueryType::NONE); }

		/// <summary> Radius for QueryType SPHERE and/or CAPSULE </summary>
		inline float QueryShapeRadius()const { return m_queryShapeSize.x; }

		/// <summary>
		/// Sets radius for QueryType SPHERE and/or CAPSULE
		/// </summary>
		/// <param name="radius"> Shape radius </param>
		inline void SetQueryShapeRadius(float radius) { m_queryShapeSize.x = std::abs(radius); }

		/// <summary> Height for QueryType CAPSULE </summary>
		inline float QueryCapsuleHeight()const { return m_queryShapeSize.y; }

		/// <summary>
		/// Sets height for QueryType CAPSULE
		/// </summary>
		/// <param name="height"> Shape height </param>
		inline void SetQueryCapsuleHeight(float height) { m_queryShapeSize.y = std::abs(height); }

		/// <summary> Size of QueryType BOX </summary>
		inline Vector3 QueryBoxSize()const { return m_queryShapeSize; }

		/// <summary>
		/// Sets QueryType BOX size
		/// </summary>
		/// <param name="size"> Shape size </param>
		inline void SetQueryBoxSize(const Vector3& size) { m_queryShapeSize = Vector3(std::abs(size.x), std::abs(size.y), std::abs(size.z)); }


		/// <summary> Layer-mask for collider filtering </summary>
		inline const Physics::PhysicsCollider::LayerMask& LayerMask()const { return m_layerMask; }

		/// <summary>
		/// Sets Layer-mask for collider filtering
		/// </summary>
		/// <param name="mask"> LayerMask </param>
		inline void SetLayerMask(const Physics::PhysicsCollider::LayerMask& mask) { m_layerMask = mask; }

		/// <summary> Generic input provider for max raycast/sweep distance </summary>
		inline Reference<InputProvider<float>> MaxDistanceInput()const { return m_maxDistance; }

		/// <summary>
		/// Sets input source for the maximal raycast/sweep distance
		/// </summary>
		/// <param name="input"> Input provider </param>
		inline void SetMaxDistanceInput(InputProvider<float>* input) { m_maxDistance = input; }

		/// <summary> Input provider for raycast/sweep origin point offset </summary>
		Reference<InputProvider<Vector3>> OriginOffsetInput()const { return m_originInput; }

		/// <summary>
		/// Sets input provider for raycast/sweep origin point offset
		/// </summary>
		/// <param name="input"> Input provider </param>
		void SetOriginOffsetInput(InputProvider<Vector3>* input) { m_originInput = input; }

		/// <summary> 
		/// Input provider for raycast/sweep direction 
		/// <para/> If this is set to nullptr, 'forward' direction will be picked by default.
		/// </summary>
		Reference<InputProvider<Vector3>> DirectionInput()const { return m_directionInput; }

		/// <summary>
		/// Sets input provider for raycast/sweep direction
		/// </summary>
		/// <param name="input"> Input provider </param>
		void SetDirectionInput(InputProvider<Vector3>* input) { m_directionInput = input; }

		/// <summary>
		/// Filter-input for filtering which colliders to ignore
		/// <para/> Input value will be used as keep/discard value in the raycast/sweep pre-filtering function
		/// </summary>
		Reference<ColliderFilterInput> ColliderFilter()const { return m_colliderFilter; }

		/// <summary>
		/// Sets filter-input for filtering which colliders to ignore
		/// </summary>
		/// <param name="input"> Input provider </param>
		void SetColliderFilterInput(ColliderFilterInput* input) { m_colliderFilter = input; }

		/// <summary>
		/// Filter-input for filtering which hit-events to ignore
		/// <para/> Input value will be used as keep/discard value in the raycast/sweep post-filtering function
		/// </summary>
		Reference<RayHitFilterInput> RayHitFilter()const { return m_rayHitFilter; }

		/// <summary>
		/// Sets filter-input for filtering which hit-events to ignore
		/// </summary>
		/// <param name="input"> Input provider </param>
		void SetRayHitFilter(RayHitFilterInput* input) { m_rayHitFilter = input; }

		/// <summary> Flags and options for the query </summary>
		inline Flags QueryFlags()const { return m_flags; }

		/// <summary>
		/// Sets query flags
		/// </summary>
		/// <param name="flags"> Flags and options for the query </param>
		inline void SetQueryFlags(const Flags flags) { m_flags = flags; }


		/// <summary> Evaluates input </summary>
		virtual std::optional<float> EvaluateInput()override;

		/// <summary> Evaluates RaycastHit input </summary>
		std::optional<RaycastHit> EvaluateRaycastHitResult();

		/// <summary>
		/// Exposes fields
		/// </summary>
		/// <param name="recordElement"> Fields will be exposed through this callback </param>
		virtual void GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement)override;


		/// <summary>
		/// [Only intended to be used by WeakReference<>; not safe for general use] Fills WeakReferenceHolder with a StrongReferenceProvider, 
		/// that will return this WeaklyReferenceable back upon request (as long as it still exists)
		/// <para/> Note that this is not thread-safe!
		/// </summary>
		/// <param name="holder"> Reference to a reference of a StrongReferenceProvider </param>
		inline virtual void FillWeakReferenceHolder(WeaklyReferenceable::WeakReferenceHolder& holder)override {
			Component::FillWeakReferenceHolder(holder);
		}

		/// <summary>
		/// [Only intended to be used by WeakReference<>; not safe for general use] Should clear the link to the StrongReferenceProvider;
		/// <para/> Address of the holder has to be the same as the one, previously passed to FillWeakReferenceHolder() method
		/// <para/> Note that this is not thread-safe!
		/// </summary>
		/// <param name="holder"> Reference to a reference of a StrongReferenceProvider </param>
		inline virtual void ClearWeakReferenceHolder(WeaklyReferenceable::WeakReferenceHolder& holder)override {
			Component::ClearWeakReferenceHolder(holder);
		}


	private:
		QueryType m_queryType = QueryType::RAY;
		Physics::PhysicsCollider::LayerMask m_layerMask = Physics::PhysicsCollider::LayerMask::All();
		WeakReference<InputProvider<float>> m_maxDistance;
		WeakReference<InputProvider<Vector3>> m_originInput;
		WeakReference<InputProvider<Vector3>> m_directionInput;
		WeakReference<ColliderFilterInput> m_colliderFilter;
		WeakReference<RayHitFilterInput> m_rayHitFilter;
		Flags m_flags = Flags::NONE;

		Vector3 m_queryShapeSize = Vector3(1.0f);

		uint64_t m_lastUpdateFrame;
		struct JIMARA_GENERIC_INPUTS_API RaycastInformation {
			WeakReference<Collider> collider;
			Vector3 point = Vector3(0.0f, 0.0f, 0.0f);
			Vector3 normal = Vector3(0.0f, 0.0f, 0.0f);
			float distance = 0.0f;
		} m_lastResult;

		struct Helpers;
	};

	// Define boolean operators for RaycastInput::Flags
	JIMARA_DEFINE_ENUMERATION_BOOLEAN_OPERATIONS(::Jimara::RaycastInput::Flags);


	// Expose type details:
	template<> inline void TypeIdDetails::GetParentTypesOf<RaycastInput_Base_RaycastHitInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<InputProvider<RaycastHit>>());
	}
	template<> inline void TypeIdDetails::GetParentTypesOf<RaycastInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<VectorInput::ComponentFrom<float>>());
		report(TypeId::Of<RaycastInput_Base_RaycastHitInput>());
	}
	template<> JIMARA_GENERIC_INPUTS_API void TypeIdDetails::GetTypeAttributesOf<RaycastInput>(const Callback<const Object*>& report);
}
