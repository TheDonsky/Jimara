#include "NavMeshAgent.h"
#include <Jimara/Data/Serialization/Helpers/SerializerMacros.h>
#include <Jimara/Environment/LogicSimulation/SimulationThreadBlock.h>
#include <Jimara/Math/Random.h>

namespace Jimara {
	struct NavMeshAgent::Helpers {
		struct RequestSnapshot {
			Reference<NavMesh> navMesh;
			AgentOptions agentOptions = {};

			std::shared_ptr<AgentState> agentState;
		};

		static bool UpdateLastKnownPositionAndUpDirection(NavMeshAgent* self) {
			std::optional<Vector3> agentPosition = InputProvider<Vector3>::GetInput(self->m_agentPositionOverride);
			std::optional<Vector3> agentUp = InputProvider<Vector3>::GetInput(self->m_agentUpDirectionOverride);
			if ((!agentPosition.has_value()) || (!agentUp.has_value())) {
				const Transform* transform = self->GetTransfrom();
				if (!agentPosition.has_value()) {
					if (transform == nullptr)
						agentUp = Vector3(0.0f);
					else agentPosition = transform->WorldPosition();
				}
				if (!agentUp.has_value())
					agentUp = (transform == nullptr) ? Math::Up() : transform->Up();
			}
			const std::optional<Vector3> targetPosition = InputProvider<Vector3>::GetInput(self->m_target);
			{
				std::unique_lock<SpinLock> lock(self->m_state->positionLock);
				self->m_state->position = agentPosition.value();
				self->m_state->up = (Math::Magnitude(agentUp.value()) > std::numeric_limits<float>::epsilon())
					? Math::Normalize(agentUp.value()) : Vector3(0.0f);
				self->m_state->targetPosition = targetPosition;
			}
			return Math::Magnitude(self->m_state->up) > 0.999f && targetPosition.has_value();
		}

		static std::optional<RequestSnapshot> CreateRequest(const NavMeshAgent* self) {
			RequestSnapshot snapshot;
			snapshot.navMesh = self->m_navMesh;
			snapshot.agentOptions = self->m_agentOptions;
			snapshot.agentState = self->m_state;
			return snapshot;
		}

		static void CalculatePath(const RequestSnapshot& snapshot) {
			NavMesh::AgentOptions options;
			options.radius = snapshot.agentOptions.radius;
			options.surfaceSearchRadius = snapshot.agentOptions.surfaceSearchRadius;
			options.maxTiltAngle = snapshot.agentOptions.angleThreshold;
			options.flags = snapshot.agentOptions.agentFlags;
			
			Vector3 agentPosition;
			Vector3 agentUp;
			std::optional<Vector3> targetPosition;
			{
				std::unique_lock<SpinLock> lock(snapshot.agentState->positionLock);
				agentPosition = snapshot.agentState->position;
				agentUp = snapshot.agentState->up;
				targetPosition = snapshot.agentState->targetPosition;
			}
			
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
				return distance * snapshot.agentOptions.slopeWeight.Value(angle);
			};
			options.additionalPathWeight = &calculateAdditionalWeight;
			std::shared_ptr<std::vector<NavMesh::PathNode>> path = std::make_shared<std::vector<NavMesh::PathNode>>();

			if (targetPosition.has_value())
				(*path) = snapshot.navMesh->CalculatePath(agentPosition, targetPosition.value(), agentUp, options);
			TrimPathNodes(snapshot.agentState.operator->(), snapshot.agentOptions.radius, *path, path->size());
			
			{
				std::unique_lock<SpinLock> lock(snapshot.agentState->pathLock);
				snapshot.agentState->path = path;
			}
		}

		struct RequestList : public virtual Object, public virtual std::vector<RequestSnapshot> {};

		struct RequestFlusher : public virtual Object {
			ThreadBlock threadBlock;
			SpinLock requestLock;
			std::unique_ptr<std::vector<Reference<RequestList>>> requests;
		};

		static void FlushRequests(Object* listPtr, float) {
			RequestFlusher& flusher = *dynamic_cast<RequestFlusher*>(listPtr);

			struct Data {
				SpinLock jobLock;
				size_t bucketId = 0u;
				size_t elemId = 0u;
				std::unique_ptr<std::vector<Reference<RequestList>>> requests;
			};
			Data data;
			{
				std::unique_lock<decltype(flusher.requestLock)> lock(flusher.requestLock);
				std::swap(flusher.requests, data.requests);
				assert(flusher.requests == nullptr);
			}
			if (data.requests == nullptr)
				return;

			typedef void(*UpdateFn)(ThreadBlock::ThreadInfo, void*);
			static const UpdateFn updateFn = [](ThreadBlock::ThreadInfo, void* procPtr) {
				Data* const proc = reinterpret_cast<Data*>(procPtr);
				while (true) {
					const RequestSnapshot* snapshot;
					{
						std::unique_lock<decltype(proc->jobLock)> lock(proc->jobLock);
						if (proc->bucketId >= proc->requests->size())
							break;
						const RequestList& requests = *proc->requests->operator[](proc->bucketId);
						if (proc->elemId >= requests.size()) {
							proc->elemId = 0u;
							proc->bucketId++;
						}
						snapshot = &requests[proc->elemId];
						proc->elemId++;
					}
					CalculatePath(*snapshot);
				}
			};

			size_t totalRequestCount = 0u;
			for (size_t i = 0u; i < data.requests->size(); i++)
				totalRequestCount += data.requests->operator[](i)->size();
			if (totalRequestCount <= 1u)
				updateFn({}, reinterpret_cast<void*>(&data));
			else flusher.threadBlock.Execute(
				Math::Min(size_t(std::thread::hardware_concurrency()) / 2u, totalRequestCount / 2u) + 1u,
				reinterpret_cast<void*>(&data), Callback<ThreadBlock::ThreadInfo, void*>(updateFn));
		}

		class Updater : public virtual ObjectCache<Reference<const Object>>::StoredObject {
		private:
			const Reference<SceneContext> m_context;
			const Reference<SimulationThreadBlock> m_threadBlock;
			std::mutex m_lock;
			std::set<NavMeshAgent*> m_agents;
			std::vector<NavMeshAgent*> m_agentList;
			const Reference<RequestFlusher> m_requestFlusher = Object::Instantiate<RequestFlusher>();

			void Update() {
				std::unique_lock<std::mutex> lock(m_lock);
				if (m_agentList.empty())
					for (auto it = m_agents.begin(); it != m_agents.end(); ++it)
						m_agentList.push_back(*it);
				if (m_agentList.empty())
					return;

				{
					std::unique_lock<decltype(m_requestFlusher->requestLock)> flusherLock(m_requestFlusher->requestLock);
					if (m_requestFlusher->requests != nullptr)
						return;
				}

				struct UpdateProcess {
					std::atomic<size_t> index = 0u;
					NavMeshAgent* const* agents = nullptr;
					size_t agentCount = 0u;
					SpinLock requestLock;
					std::unique_ptr<std::vector<Reference<RequestList>>> requests =
						std::make_unique<std::vector<Reference<RequestList>>>();
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
					const Reference<RequestList> requestList = Object::Instantiate<RequestList>();
					while (true) {
						const size_t index = proc->index.fetch_add(1u);
						if (index >= proc->agentCount)
							break;
						NavMeshAgent* agent = proc->agents[index];
						if (!UpdateLastKnownPositionAndUpDirection(agent))
							continue;
						TrimPath(agent);
						const uint64_t frameId = agent->Context()->FrameIndex();
						if (frameId < agent->m_updateFrame &&
							(agent->m_updateFrame - frameId) <= agent->m_updateInterval)
							continue;
						std::optional<RequestSnapshot> snapshot = CreateRequest(agent);
						if (!snapshot.has_value())
							continue;
						agent->m_updateFrame = frameId + uint64_t(Random::Uint()) % (uint64_t(agent->m_updateInterval) + 1u) + 1u;
						requestList->emplace_back(std::move(snapshot.value()));
					}
					if (!requestList->empty()) {
						std::unique_lock<decltype(proc->requestLock)> lock(proc->requestLock);
						proc->requests->push_back(requestList);
					}
				};
				if (process.agentCount < 32u)
					updateFn({}, reinterpret_cast<void*>(&process));
				else m_threadBlock->Execute(Math::Min(m_threadBlock->DefaultThreadCount(), process.agentCount / 16u + 1u),
					reinterpret_cast<void*>(&process), Callback<ThreadBlock::ThreadInfo, void*>(updateFn));
				
				if (!process.requests->empty()) {
					const Reference<NavMesh> navMesh = process.requests->begin()->operator->()->begin()->navMesh;
					assert(navMesh != nullptr);
					{
						std::unique_lock<decltype(m_requestFlusher->requestLock)> flusherLock(m_requestFlusher->requestLock);
						std::swap(m_requestFlusher->requests, process.requests);
					}
					navMesh->EnqueueAsynchronousAction(
						Callback<Object*, float>(Helpers::FlushRequests), m_requestFlusher);
				}
			}

		public:
			inline Updater(SceneContext* ctx) 
				: m_context(ctx)
				, m_threadBlock(SimulationThreadBlock::GetFor(ctx))  {
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

		static void OnEnabledOrDisabled(NavMeshAgent* self) {
			Reference<Updater> updater = self->m_updater;
			if (self->ActiveInHierarchy())
				updater->Add(self);
			else updater->Remove(self);
			self->m_updateFrame = self->Context()->FrameIndex() + uint64_t(Random::Uint()) % (uint64_t(self->m_updateInterval) + 1u) + 1u;
		}

		inline static void TrimPath(const NavMeshAgent* self) {
			std::shared_ptr<std::vector<NavMesh::PathNode>> pathPtr;
			{
				std::unique_lock<SpinLock> lock(self->m_state->pathLock);
				pathPtr = self->m_state->path;
			}
			if (pathPtr == nullptr)
				return;
			TrimPathNodes(self->m_state.operator->(), self->m_agentOptions.radius, *pathPtr, 2u);
		}

		inline static void TrimPathNodes(AgentState* state, float radius, std::vector<NavMesh::PathNode>& path, size_t checkedEdgeCount) {
			if (path.size() < 2u)
				return;

			Vector3 position;
			Vector3 agentUp;
			{
				std::unique_lock<SpinLock> lock(state->positionLock);
				position = state->position;
				agentUp = state->up;
			}
			if (Math::Magnitude(agentUp) < 0.999f)
				return;

			for (size_t endId = Math::Min(path.size() - 1u, checkedEdgeCount); endId > 0u; endId--) {
				const size_t startId = endId - 1u;
				NavMesh::PathNode& start = path[startId];
				const NavMesh::PathNode& end = path[endId];
				
				const Vector3 segmentDelta = (end.position - start.position);
				const float segmentSize = Math::Magnitude(segmentDelta);
				if (segmentSize < std::numeric_limits<float>::epsilon())
					continue;
				const Vector3 segmentDir = segmentDelta / segmentSize;

				const Vector3 offsetFromStart = (position - start.position);
				const float progressOnAxis = Math::Dot(offsetFromStart, segmentDir);
				const Vector3 offsetFromAxis = offsetFromStart - segmentDir * progressOnAxis;
				const float distanceFromAxis = Math::Magnitude(offsetFromAxis);
				
				if (startId > 0u) {
					if (distanceFromAxis >= radius ||
						std::abs(progressOnAxis) >= (Math::Min(radius, segmentSize) * 0.5f))
						continue;
				}
				else if (std::abs(segmentSize - progressOnAxis) < radius)
					continue;
				
				const float phase = progressOnAxis / segmentSize;
				start.normal = Math::Normalize(start.normal * (1.0f - progressOnAxis) + end.normal * progressOnAxis);
				if (startId > 0u)
					start.position += segmentDir * progressOnAxis;
				else {
					const float cosine = Math::Max(std::abs(Math::Dot(start.normal, agentUp)), 0.00001f);
					start.position = position - agentUp * (distanceFromAxis / cosine);
				}

				if (startId > 0u) {
					const size_t trimmedNodeCount = (path.size() - startId);
					for (size_t i = 0u; i < trimmedNodeCount; i++)
						path[i] = path[startId + i];
					path.resize(trimmedNodeCount);
				}
				break;
			}
		}
	};

	NavMeshAgent::NavMeshAgent(Component* parent, const std::string_view& name)
		: Component(parent, name)
		, m_navMesh(NavMesh::Instance(parent->Context()))
		, m_updater(Helpers::Updater::GetFor(parent->Context())) {
	}

	NavMeshAgent::~NavMeshAgent() {
		assert(Destroyed());
		Helpers::OnEnabledOrDisabled(this);
	}

	std::shared_ptr<const std::vector<NavMesh::PathNode>> NavMeshAgent::Path()const {
		std::unique_lock<SpinLock> lock(m_state->pathLock);
		const std::shared_ptr<const std::vector<NavMesh::PathNode>> path = m_state->path;
		return path;
	}

	std::optional<Vector3> NavMeshAgent::EvaluateInput() {
		const std::shared_ptr<const std::vector<NavMesh::PathNode>> path = Path();
		if (path == nullptr || path->size() < 2u)
			return std::nullopt;
		const std::vector<NavMesh::PathNode>& nodes = *path;
		return Math::Normalize(nodes[1u].position - nodes[0u].position);
	}

	void NavMeshAgent::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		Component::GetFields(recordElement);
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			JIMARA_SERIALIZE_FIELD_GET_SET(Target, SetTarget, "Target", "Target point input");
			JIMARA_SERIALIZE_FIELD_GET_SET(AgentPositionOverride, SetAgentPositionOverride, "Position Override", 
				"Optional override for agent position \n"
				"If not provided, transform position will be picked by default.");
			JIMARA_SERIALIZE_FIELD_GET_SET(AgentUpDirectionOverride, SetAgentUpDirectionOverride, "Up Direction Override", 
				"Optional override for agent up-direction \n"
				"If not provided, transform-up will be picked by default; If there is no transform, just Y direction is the default fallback");
			JIMARA_SERIALIZE_FIELD_GET_SET(Radius, SetRadius, "Radius", "Agent radius");
			JIMARA_SERIALIZE_FIELD_GET_SET(MaxTiltAngle, SetMaxTiltAngle, "Max Tilt Angle", "Maximal slope angle the agent can climb");
			{
				bool walkOnWalls = ((m_agentOptions.agentFlags & NavMesh::AgentFlags::FIXED_UP_DIRECTION) == NavMesh::AgentFlags::NONE);
				JIMARA_SERIALIZE_FIELD(walkOnWalls, "Walk On Walls",
					"If set, this flag lets the agent 'walk on walls', as long as individual surfaces have angle lesser than Max Tilt Angle");
				if (walkOnWalls) m_agentOptions.agentFlags = m_agentOptions.agentFlags & static_cast<NavMesh::AgentFlags>(
					~static_cast<std::underlying_type_t<NavMesh::AgentFlags>>(NavMesh::AgentFlags::FIXED_UP_DIRECTION));
				else m_agentOptions.agentFlags = m_agentOptions.agentFlags | NavMesh::AgentFlags::FIXED_UP_DIRECTION;
			}
			JIMARA_SERIALIZE_FIELD(m_agentOptions.slopeWeight, "Slope Weight", "Additional weight fraction per slope angle",
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
