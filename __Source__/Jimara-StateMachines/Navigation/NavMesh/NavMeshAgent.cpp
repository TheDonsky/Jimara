#include "NavMeshAgent.h"
#include <Jimara/Data/Serialization/Helpers/SerializerMacros.h>
#include <Jimara/Environment/LogicSimulation/SimulationThreadBlock.h>
#include <Jimara/Math/Random.h>

namespace Jimara {
	struct NavMeshAgent::Helpers {
		static void RandomizeNextUpdate(NavMeshAgent* self) {
			self->m_updateFrame = self->Context()->FrameIndex() + 1u;
		}

		static void RecalculatePathIfNeeded(NavMeshAgent* self) {
			if (self->Context()->FrameIndex() >= self->m_updateFrame)
				self->RecalculatePath();
		}

		static void OnEnabledOrDisabled(NavMeshAgent* self) {
			const Callback<> action = Callback(Helpers::RecalculatePathIfNeeded, self);
			self->Context()->Graphics()->OnGraphicsSynch() -= action;
			if (self->ActiveInHeirarchy())
				self->Context()->Graphics()->OnGraphicsSynch() += action;
			self->RecalculatePath();
			self->m_updateFrame = self->Context()->FrameIndex() + Random::Uint() % (self->m_updateInterval + 1u) + 1u;
		}
	};

	NavMeshAgent::NavMeshAgent(Component* parent, const std::string_view& name)
		: Component(parent, name)
		, m_navMesh(NavMesh::Instance(parent->Context())) {}

	NavMeshAgent::~NavMeshAgent() {
		assert(Destroyed());
		Helpers::OnEnabledOrDisabled(this);
	}

	void NavMeshAgent::RecalculatePath() {
		m_path.clear();
		const Transform* transform = GetTransfrom();
		const Reference<InputProvider<Vector3>> target = m_target;
		if (transform == nullptr || target == nullptr)
			return;
		std::optional<Vector3> targetPoint = target->GetInput();
		if (!targetPoint.has_value())
			return;
		NavMesh::AgentOptions options;
		options.radius = m_radius;
		options.maxTiltAngle = m_angleThreshold;
		options.flags = m_agentFlags;
		m_path = m_navMesh->CalculatePath(transform->WorldPosition(), targetPoint.value(), transform->Up(), options);
		m_updateFrame = Context()->FrameIndex() + m_updateInterval + 1u;
	}

	std::optional<Vector3> NavMeshAgent::EvaluateInput() {
		Helpers::RecalculatePathIfNeeded(this);
		if (m_path.size() < 2u)
			return {};
		return Math::Normalize(m_path[1u].position - m_path[0u].position);
	}

	void NavMeshAgent::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		Component::GetFields(recordElement);
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			JIMARA_SERIALIZE_FIELD_GET_SET(Target, SetTarget, "Target", "Target point input");
			JIMARA_SERIALIZE_FIELD_GET_SET(Radius, SetRadius, "Radius", "Agent radius");
			JIMARA_SERIALIZE_FIELD_GET_SET(MaxTiltAngle, SetMaxTiltAngle, "Max Tilt Angle", "Maximal slope angle the agent can climb");
			{
				bool walkOnWalls = ((m_agentFlags & NavMesh::AgentFlags::FIXED_UP_DIRECTION) == NavMesh::AgentFlags::NONE);
				JIMARA_SERIALIZE_FIELD(walkOnWalls, "Walk On Walls",
					"If set, this flag lets the agent 'walk on walls', as long as individual surfaces have angle lesser than Max Tilt Angle");
				if (walkOnWalls) m_agentFlags = m_agentFlags & static_cast<NavMesh::AgentFlags>(
					~static_cast<std::underlying_type_t<NavMesh::AgentFlags>>(NavMesh::AgentFlags::FIXED_UP_DIRECTION));
				else m_agentFlags = m_agentFlags | NavMesh::AgentFlags::FIXED_UP_DIRECTION;
			}
			JIMARA_SERIALIZE_FIELD_GET_SET(UpdateInterval, SetUpdateInterval, "Update interval",
				"Number of idle frames in-between path recalculations");
		};
	}

	void NavMeshAgent::OnComponentEnabled() {
		Helpers::OnEnabledOrDisabled(this);
	}

	void NavMeshAgent::OnComponentDisabled() {
		Helpers::OnEnabledOrDisabled(this);
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<NavMeshAgent>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<NavMeshAgent>(
			"Nav-Mesh Agent", "Jimara/Navigation/Nav-Mesh Agent", "Navigation Mesh agent");
		report(factory);
	}
}
