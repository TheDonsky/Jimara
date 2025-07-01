#include "RaycastInput.h"
#include <Jimara/Environment/Layers.h>
#include <Jimara/Data/Serialization/Helpers/SerializerMacros.h>


namespace Jimara {
	struct RaycastInput::Helpers {
		static void Update(RaycastInput* self) {
			const uint64_t frameId = self->Context()->FrameIndex();
			if ((frameId == self->m_lastUpdateFrame) && 
				((self->m_flags & Flags::DISABLE_FRAME_CACHING) == Flags::NONE))
				return;
			self->m_lastUpdateFrame = frameId;
			self->m_lastResult.collider = nullptr;

			const float inputMaxDistance = InputProvider<float>::GetInput(self->m_maxDistance, std::numeric_limits<float>::epsilon());
			if (std::isnan(inputMaxDistance) || inputMaxDistance <= 0.0f)
				return;

			const Vector3 inputOrigin = InputProvider<Vector3>::GetInput(self->m_originInput, Vector3(0.0f, 0.0f, 0.0f));
			const Vector3 inputDirection = InputProvider<Vector3>::GetInput(self->m_directionInput, Math::Forward());

			const bool originLocalSpace = (self->m_flags & Flags::QUERY_ORIGIN_WORLD_SPACE) == Flags::NONE;
			const bool directionLocalSpace = (self->m_flags & Flags::QUERY_DIRECTION_WORLD_SPACE) == Flags::NONE;
			const bool scaleMaxDistance = (self->m_flags & Flags::SCALE_MAX_DISTANCE_BY_LOSSY_SCALE) != Flags::NONE;
			
			const bool scaleSweepShape =
				((self->m_flags & Flags::SCALE_SWEEP_SHAPE_BY_LOSSY_SCALE) != Flags::NONE) && (self->m_queryType != QueryType::RAY);
			const bool rotateSweepShape =
				((self->m_flags & Flags::DO_NOT_ROTATE_SWEEP_SHAPE) == Flags::NONE) && (self->m_queryType != QueryType::RAY);

			const bool transformNeeded = (originLocalSpace || directionLocalSpace || scaleMaxDistance || scaleSweepShape || rotateSweepShape);
			const bool rotationMatrixNeeded = (scaleMaxDistance || scaleSweepShape || rotateSweepShape);
			const bool scaleNeeded = (scaleMaxDistance || scaleSweepShape);

			const Transform* transform = (transformNeeded) ? self->GetTransfrom() : nullptr;
			const Matrix4 worldMatrix = (transform != nullptr) ? transform->WorldMatrix() : Math::Identity();
			const Matrix4 worldRotationMatrix = (transform != nullptr && rotationMatrixNeeded) ? transform->WorldRotationMatrix() : Math::Identity();
			const Vector3 lossyScaleRaw = scaleNeeded ? Math::LossyScale(worldMatrix, worldRotationMatrix) : Vector3(1.0f);
			const Vector3 lossyScale = Vector3(std::abs(lossyScaleRaw.x), std::abs(lossyScaleRaw.y), std::abs(lossyScaleRaw.z));
			
			const Vector3 origin = originLocalSpace ? Vector3(worldMatrix * Vector4(inputOrigin, 1.0f)) : inputOrigin;
			const Vector3 direction = Math::Normalize(directionLocalSpace ? Vector3(worldMatrix * Vector4(inputDirection, 0.0f)) : inputDirection);

			const float maxDistance = scaleMaxDistance
				? Math::Magnitude(
					lossyScale * inputMaxDistance *
					Math::Normalize(directionLocalSpace
						? inputDirection
						: Vector3(Math::Inverse(worldMatrix) * Vector4(inputDirection, 0.0f))))
				: inputMaxDistance;

			auto filterFn = [&](auto filter, auto data) -> Physics::PhysicsScene::QueryFilterFlag {
				if (filter == nullptr)
					return Physics::PhysicsScene::QueryFilterFlag::REPORT;
				std::optional<bool> result = filter->GetInput(data);
				if ((!result.has_value()) || result.value())
					return Physics::PhysicsScene::QueryFilterFlag::DISCARD;
				return Physics::PhysicsScene::QueryFilterFlag::REPORT;
			};

			Reference<ColliderFilterInput> colliderFilter = self->m_colliderFilter;
			auto filterCollider = [&](Collider* collider) { return filterFn(colliderFilter, collider); };
			const Function<Physics::PhysicsScene::QueryFilterFlag, Collider*> preFilterFn =
				Function<Physics::PhysicsScene::QueryFilterFlag, Collider*>::FromCall(&filterCollider);
			const Function<Physics::PhysicsScene::QueryFilterFlag, Collider*>* preFilter = (colliderFilter != nullptr) ? &preFilterFn : nullptr;

			Reference<RayHitFilterInput> rayHitFilter = self->m_rayHitFilter;
			auto filterRayHit = [&](const RaycastHit& hit) { return filterFn(rayHitFilter, hit); };
			const Function<Physics::PhysicsScene::QueryFilterFlag, const RaycastHit&> postFilterFn =
				Function<Physics::PhysicsScene::QueryFilterFlag, const RaycastHit&>::FromCall(&filterRayHit);
			const Function<Physics::PhysicsScene::QueryFilterFlag, const RaycastHit&>* postFilter = (rayHitFilter != nullptr) ? &postFilterFn : nullptr;

			static_assert(static_cast<uint64_t>(Flags::EXCLUDE_DYNAMIC_BODIES) == static_cast<uint64_t>(Physics::PhysicsScene::QueryFlag::EXCLUDE_DYNAMIC_BODIES));
			static_assert(static_cast<uint64_t>(Flags::EXCLUDE_STATIC_BODIES) == static_cast<uint64_t>(Physics::PhysicsScene::QueryFlag::EXCLUDE_STATIC_BODIES));
			const Physics::PhysicsScene::QueryFlags queryFlags = static_cast<Physics::PhysicsScene::QueryFlags>(
				self->m_flags & (Flags::EXCLUDE_DYNAMIC_BODIES | Flags::EXCLUDE_STATIC_BODIES));

			bool found = false;
			auto onHitFound = [&](const RaycastHit& hit) {
				assert(hit.collider != nullptr);
				if (found)
					self->Context()->Log()->Error("RaycastInput - Internal Error: More than one hit reported! [File", __FILE__, "; Line: ", __LINE__, "]");
				found = true;
				self->m_lastResult.collider = hit.collider;
				self->m_lastResult.point = hit.point;
				self->m_lastResult.normal = hit.normal;
				self->m_lastResult.distance = hit.distance;
			};

			auto query = [&](auto queryFn) {
				queryFn(direction, maxDistance,
					Callback<const RaycastHit&>::FromCall(&onHitFound),
					self->m_layerMask, queryFlags,
					preFilter, postFilter);
			};
			auto sweep = [&](const auto& shape) {
				Matrix4 poseMatrix = rotateSweepShape ? worldRotationMatrix : Math::Identity();
				poseMatrix[3u] = Vector4(origin, 1.0f);
				query([&](const auto&... args) { self->Context()->Physics()->Sweep(shape, poseMatrix, args...); });
			};

			static const constexpr float EPS = std::numeric_limits<float>::epsilon() * 16.0f;

			switch (self->m_queryType) {
			case QueryType::RAY:
				query([&](auto... args) { self->Context()->Physics()->Raycast(origin, args...); });
				break;
			case QueryType::SPHERE:
			{
				const float rawRadius = self->QueryShapeRadius();
				const float radius = scaleSweepShape
					? (Math::Max(lossyScale.x, lossyScale.y, lossyScale.z) * rawRadius)
					: rawRadius;
				if (std::abs(radius) > EPS) {
					Physics::SphereShape shape(std::abs(radius));
					sweep(shape);
				}
				else query([&](auto... args) { self->Context()->Physics()->Raycast(origin, args...); });
				break;
			}
			case QueryType::CAPSULE:
			{
				const float rawRadius = self->QueryShapeRadius();
				const float rawHeight = self->QueryCapsuleHeight();
				const float scale = scaleSweepShape ? Math::Max(lossyScale.x, lossyScale.y, lossyScale.z) : 1.0f;
				const float radius = Math::Max(std::abs(rawRadius * scale), EPS);
				const float height = Math::Max(std::abs(rawHeight * scale), EPS);
				Physics::CapsuleShape shape(radius, height);
				sweep(shape);
				break;
			}
			case QueryType::BOX:
			{
				const Vector3 rawBoxSize = self->QueryBoxSize();
				const Vector3 boxSize = rawBoxSize * lossyScale;
				Physics::BoxShape shape(Vector3(
					Math::Max(std::abs(boxSize.x), EPS),
					Math::Max(std::abs(boxSize.y), EPS),
					Math::Max(std::abs(boxSize.z), EPS)));
				sweep(shape);
				break;
			}
			default:
				break;
			}
		}
	};


	const Object* RaycastInput::QueryTypeEnumerationAttribute() {
		static const auto attribute = Object::Instantiate<Serialization::EnumAttribute<std::underlying_type_t<QueryType>>>(false,
			"RAY", QueryType::RAY,
			"SPHERE", QueryType::SPHERE,
			"CAPSULE", QueryType::CAPSULE,
			"BOX", QueryType::BOX);
		return attribute;
	}

	const Object* RaycastInput::FlagOptionsEnumerationAttribute() {
		static const auto attribute = Object::Instantiate<Serialization::EnumAttribute<std::underlying_type_t<Flags>>>(true,
			"QUERY_WHEN_DISABLED", Flags::QUERY_WHEN_DISABLED,
			"EXCLUDE_DYNAMIC_BODIES", Flags::EXCLUDE_DYNAMIC_BODIES,
			"EXCLUDE_STATIC_BODIES", Flags::EXCLUDE_STATIC_BODIES,
			"QUERY_ORIGIN_WORLD_SPACE", Flags::QUERY_ORIGIN_WORLD_SPACE,
			"QUERY_DIRECTION_WORLD_SPACE", Flags::QUERY_DIRECTION_WORLD_SPACE,
			"SCALE_MAX_DISTANCE_BY_LOSSY_SCALE", Flags::SCALE_MAX_DISTANCE_BY_LOSSY_SCALE,
			"SCALE_SWEEP_SHAPE_BY_LOSSY_SCALE", Flags::SCALE_SWEEP_SHAPE_BY_LOSSY_SCALE,
			"DO_NOT_ROTATE_SWEEP_SHAPE", Flags::DO_NOT_ROTATE_SWEEP_SHAPE,
			"DISABLE_FRAME_CACHING", Flags::DISABLE_FRAME_CACHING);
		return attribute;
	}


	RaycastInput::RaycastInput(Component* parent, const std::string_view& name)
		: Component(parent, name)
		, m_lastUpdateFrame(parent->Context()->FrameIndex() - 1u) {}

	std::optional<float> RaycastInput::EvaluateInput() {
		std::optional<RaycastHit> res = EvaluateRaycastHitResult();
		if (res == std::nullopt)
			return std::nullopt;
		else return res->distance;
	}

	std::optional<RaycastHit> RaycastInput_Base_RaycastHitInput::GetInput() {
		RaycastInput* self = dynamic_cast<RaycastInput*>(this);
		assert(self != nullptr);
		return self->EvaluateRaycastHitResult();
	}

	std::optional<RaycastHit> RaycastInput::EvaluateRaycastHitResult() {
		if (((m_flags & Flags::QUERY_WHEN_DISABLED) == Flags::NONE) && (!ActiveInHierarchy()))
			return std::nullopt;
		Helpers::Update(this);
		Reference<Collider> collider = m_lastResult.collider;
		if (collider == nullptr)
			return std::nullopt;
		RaycastHit hit = {};
		hit.collider = collider;
		hit.point = m_lastResult.point;
		hit.normal = m_lastResult.normal;
		hit.distance = m_lastResult.distance;
		return hit;
	}

	void RaycastInput::GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement) {
		Jimara::Component::GetFields(recordElement);
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			JIMARA_SERIALIZE_FIELD_GET_SET(QueryMode, SetQueryMode, "Query Mode", "Type of the raycast or a sweep", QueryTypeEnumerationAttribute());
			switch (QueryMode()) {
			case QueryType::RAY:
				break;
			case QueryType::SPHERE:
				JIMARA_SERIALIZE_FIELD_GET_SET(QueryShapeRadius, SetQueryShapeRadius, "Sphere Radius", "Radius of the swept sphere");
				break;
			case QueryType::CAPSULE:
				JIMARA_SERIALIZE_FIELD_GET_SET(QueryShapeRadius, SetQueryShapeRadius, "Capsule Radius", "Radius of the swept capsule");
				JIMARA_SERIALIZE_FIELD_GET_SET(QueryCapsuleHeight, SetQueryCapsuleHeight, "Capsule Height", "Height of the swept capsule");
				break;
			case QueryType::BOX:
				JIMARA_SERIALIZE_FIELD_GET_SET(QueryBoxSize, SetQueryBoxSize, "Box Size", "Size of the swept box");
				break;
			default:
				break;
			}
			JIMARA_SERIALIZE_FIELD(m_layerMask, "Layer Mask", "Layer Mask for filtering colliders", Layers::LayerMaskAttribute::Instance());
			JIMARA_SERIALIZE_FIELD(m_maxDistance, "Max Distance", "Generic input provider for max raycast/sweep distance");
			JIMARA_SERIALIZE_FIELD(m_originInput, "Origin Offset", "Input provider for raycast/sweep origin point offset");
			JIMARA_SERIALIZE_FIELD(m_directionInput, "Direction input", 
				"Input provider for raycast/sweep direction\n"
				"If this is set to nullptr, 'forward' direction will be picked by default.");
			JIMARA_SERIALIZE_FIELD(m_colliderFilter, "Collider Filter", 
				"Filter-input for filtering which colliders to ignore\n"
				"Input value will be used as keep/discard value in the raycast/sweep pre-filtering function.");
			JIMARA_SERIALIZE_FIELD(m_rayHitFilter, "Ray-Hit Filter", 
				"Filter-input for filtering which hit-events to ignore\n"
				"Input value will be used as keep/discard value in the raycast/sweep post-filtering function.");
			JIMARA_SERIALIZE_FIELD_GET_SET(QueryFlags, SetQueryFlags, "Query Flags", "Flags and options for the query", FlagOptionsEnumerationAttribute());
		};
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<RaycastInput>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<RaycastInput>(
			"Raycast Input", "Jimara/Input/Physical/RaycastInput",
			"An input provider that performs a raycast/sweep and returns RaycastHit and/or hit-distance\n"
			"Floating-point-type Vector-input evaluates hit distance (nullopt if there's no hit);\n"
			"RaycastHit input provider interface evaluates the hit itself.");
		report(factory);
	}
}
