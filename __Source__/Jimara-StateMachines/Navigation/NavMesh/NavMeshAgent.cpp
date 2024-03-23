#include "NavMeshAgent.h"
#include <Jimara/Data/Serialization/Helpers/SerializerMacros.h>
#include <Jimara/Environment/LogicSimulation/SimulationThreadBlock.h>
#include <Jimara/Math/Random.h>

namespace Jimara {
	struct NavMeshAgent::Helpers {
		class Updater : public virtual ObjectCache<Reference<const Object>>::StoredObject {
		private:
			const Reference<SceneContext> m_context;
			const Reference<SimulationThreadBlock> m_threadBlock;
			std::mutex m_lock;
			std::set<NavMeshAgent*> m_agents;
			std::vector<NavMeshAgent*> m_agentList;

			void Update() {
				std::unique_lock<std::mutex> lock(m_lock);
				if (m_agentList.empty())
					for (auto it = m_agents.begin(); it != m_agents.end(); ++it)
						m_agentList.push_back(*it);
				if (m_agentList.empty())
					return;

				struct UpdateProcess {
					std::atomic<size_t> index = 0u;
					NavMeshAgent* const* agents = nullptr;
					size_t agentCount = 0u;
				};
				UpdateProcess process = {};
				{
					process.index = 0u;
					process.agents = m_agentList.data();
					process.agentCount = m_agentList.size();
				}
				typedef void(*UpdateFn)(ThreadBlock::ThreadInfo, void*);
				static const UpdateFn updateFn = [](ThreadBlock::ThreadInfo, void* procPtr) {
					UpdateProcess* proc = reinterpret_cast<UpdateProcess*>(procPtr);
					while (true) {
						const size_t index = proc->index.fetch_add(1u);
						if (index >= proc->agentCount)
							break;
						RecalculatePathIfNeeded(proc->agents[index]);
					}
				};
				if (process.agentCount < 4u)
					updateFn({}, reinterpret_cast<void*>(&process));
				else m_threadBlock->Execute(Math::Min(m_threadBlock->DefaultThreadCount(), process.agentCount),
					reinterpret_cast<void*>(&process), Callback<ThreadBlock::ThreadInfo, void*>(updateFn));
			}

		public:
			inline Updater(SceneContext* ctx) 
				: m_context(ctx)
				, m_threadBlock(SimulationThreadBlock::GetFor(ctx)) {
				m_context->OnSynchOrUpdate() += Callback<>(&Updater::Update, this);
			}
			inline virtual ~Updater() {
				m_context->OnSynchOrUpdate() -= Callback<>(&Updater::Update, this);
			}

			static Reference<Updater> GetFor(SceneContext* context) {
				if (context == nullptr)
					return nullptr;
				struct Cache : public virtual ObjectCache<Reference<const Object>> {
					static Reference<Updater> Get(SceneContext* ctx) {
						static Cache cache;
						return cache.GetCachedOrCreate(ctx, [&]() { return Object::Instantiate<Updater>(ctx); });
					}
				};
				return Cache::Get(context);
			}

			inline void Add(NavMeshAgent* agent) {
				std::unique_lock<std::mutex> lock(m_lock);
				m_agents.insert(agent);
				m_agentList.clear();
			}
			inline void Remove(NavMeshAgent* agent) {
				std::unique_lock<std::mutex> lock(m_lock);
				m_agents.erase(agent);
				m_agentList.clear();
			}
		};

		static void RandomizeNextUpdate(NavMeshAgent* self) {
			self->m_updateFrame = self->Context()->FrameIndex() + 1u;
		}

		static void RecalculatePathIfNeeded(NavMeshAgent* self) {
			if (self->Context()->FrameIndex() >= self->m_updateFrame)
				self->RecalculatePath();
		}

		static void OnEnabledOrDisabled(NavMeshAgent* self) {
			Reference<Updater> updater = self->m_updater;
			if (self->ActiveInHeirarchy())
				updater->Add(self);
			else updater->Remove(self);
			self->RecalculatePath();
			self->m_updateFrame = self->Context()->FrameIndex() + Random::Uint() % (self->m_updateInterval + 1u) + 1u;
		}
	};

	NavMeshAgent::NavMeshAgent(Component* parent, const std::string_view& name)
		: Component(parent, name)
		, m_navMesh(NavMesh::Instance(parent->Context()))
		, m_updater(Helpers::Updater::GetFor(parent->Context())) {}

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
		const Vector3 agentUp = transform->Up();
		NavMesh::AgentOptions options;
		options.radius = m_radius;
		options.maxTiltAngle = m_angleThreshold;
		options.flags = m_agentFlags;
		auto calculateAdditionalWeight = [&](const NavMesh::PathNode& a, const NavMesh::PathNode& b) {
			auto getAngle = [&](const Vector3& nA, const Vector3& nB) {
				return Math::Degrees(std::acos(Math::Max(Math::Min(Math::Dot(nA, nB), 1.0f), 0.0f)));
			};
			float angle;
			if ((options.flags & NavMesh::AgentFlags::FIXED_UP_DIRECTION) != NavMesh::AgentFlags::NONE) {
				const Vector3 averageNormal = Math::Normalize(a.normal + b.normal);
				angle = getAngle(averageNormal, agentUp);
				if (Math::Dot(agentUp, a.position) > Math::Dot(agentUp, b.position))
					angle = -angle; // Sloping down...
			}
			else angle = getAngle(a.normal, b.normal);
			const float distance = Math::Magnitude(a.position - b.position);
			return distance * m_slopeWeight.Value(angle);
		};
		options.additionalPathWeight = &calculateAdditionalWeight;
		m_path = m_navMesh->CalculatePath(transform->WorldPosition(), targetPoint.value(), agentUp, options);
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
			JIMARA_SERIALIZE_FIELD(m_slopeWeight, "Slope Weight", "Additional weight fraction per slope angle",
				Object::Instantiate<Serialization::CurveGraphCoordinateLimits>(-180.0f, 180.0f, 0.0f));
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
