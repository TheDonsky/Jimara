#include "NavMeshAgent.h"
#include <Jimara/Data/Serialization/Helpers/SerializerMacros.h>


namespace Jimara {
	struct NavMeshAgent::Helpers {
		static void OnEnabledOrDisabled(NavMeshAgent* self) {
			const Callback<> action = Callback(&NavMeshAgent::RecalculatePath, self);
			self->Context()->Graphics()->OnGraphicsSynch() -= action;
			if (self->ActiveInHeirarchy())
				self->Context()->Graphics()->OnGraphicsSynch() += action;
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
		m_path = m_navMesh->CalculatePath(transform->WorldPosition(), targetPoint.value(), transform->Up(), options);
	}

	std::optional<Vector3> NavMeshAgent::EvaluateInput() {
		if (m_path.size() < 2u)
			return {};
		return Math::Normalize(m_path[1u].position - m_path[0u].position);
	}

	void NavMeshAgent::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		Component::GetFields(recordElement);
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			JIMARA_SERIALIZE_FIELD_GET_SET(Target, SetTarget, "Target", "Target point input");
			JIMARA_SERIALIZE_FIELD_GET_SET(Radius, SetRadius, "Radius", "Agent radius");
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
