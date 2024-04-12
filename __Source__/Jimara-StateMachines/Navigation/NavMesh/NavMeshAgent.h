#pragma once
#include "NavMesh.h"
#include <Jimara/Math/Curves.h>
#include <Jimara/Core/Systems/InputProvider.h>
#include <Jimara-GenericInputs/Base/VectorInput.h>


namespace Jimara {
	JIMARA_REGISTER_TYPE(Jimara::NavMeshAgent);

	/// <summary>
	/// Navigation mesh agent, for computing paths with certain frequency
	/// </summary>
	class JIMARA_STATE_MACHINES_API NavMeshAgent 
		: public virtual VectorInput::ComponentFrom<Vector3> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Name of the agent </param>
		NavMeshAgent(Component* parent, const std::string_view& name = "NavMeshAgent");

		/// <summary> Virtual destructor </summary>
		virtual ~NavMeshAgent();

		/// <summary> Target position input </summary>
		inline InputProvider<Vector3>* Target()const { 
			return m_target.operator Jimara::Reference<Jimara::InputProvider<Jimara::Vector3>>(); 
		}

		/// <summary>
		/// Sets target input position
		/// </summary>
		/// <param name="target"> Target position input </param>
		inline void SetTarget(InputProvider<Vector3>* target) { m_target = target; }

		/// <summary> Optional override for agent position (if not provided, transform position will be picked by default) </summary>
		inline InputProvider<Vector3>* AgentPositionOverride()const {
			return m_agentPositionOverride.operator Jimara::Reference<Jimara::InputProvider<Jimara::Vector3>>();
		}

		/// <summary>
		/// Sets agent position override input
		/// <para/> nullptr will result in transform position being used for calculations.
		/// </summary>
		/// <param name="target"> Agent position override </param>
		inline void SetAgentPositionOverride(InputProvider<Vector3>* override) { m_agentPositionOverride = override; }

		/// <summary> Optional override for agent up-direction (if not provided, transform-up will be picked by default) </summary>
		inline InputProvider<Vector3>* AgentUpDirectionOverride()const {
			return m_agentUpDirectionOverride.operator Jimara::Reference<Jimara::InputProvider<Jimara::Vector3>>();
		}

		/// <summary>
		/// Sets agent position override input
		/// <para/> nullptr or nullopt values will result in transform-up direction being used for calculations 
		/// (if no transform is present in parents, just up direction is the default).
		/// </summary>
		/// <param name="target"> Agent up-direction override </param>
		inline void SetAgentUpDirectionOverride(InputProvider<Vector3>* override) { m_agentUpDirectionOverride = override; }

		/// <summary> Last path that has been calculated </summary>
		std::shared_ptr<const std::vector<NavMesh::PathNode>> Path()const;

		/// <summary> Agent radius </summary>
		inline float Radius()const { return m_agentOptions.radius.load(); }

		/// <summary>
		/// Sets agent radius
		/// </summary>
		/// <param name="radius"> Radius to use </param>
		inline void SetRadius(float radius) { m_agentOptions.radius = Math::Max(radius, 0.0f); }

		/// <summary> 
		/// Radius for searching start and end points (start and end face search is efeected by this, but pathfinding does not care) 
		/// </summary>
		inline float SurfaceSearchRadius()const { return m_agentOptions.surfaceSearchRadius; }

		/// <summary>
		/// Sets surface search radius
		/// </summary>
		/// <param name="radius"> Search radius </param>
		inline void SetSurfaceSearchRadius(float radius) { m_agentOptions.surfaceSearchRadius = Math::Max(radius, 0.0f); }

		/// <summary> Maximal slope the agent can go on (if it can climb walls, this angle becomes angle between two surface faces) </summary>
		inline float MaxTiltAngle()const { return m_agentOptions.angleThreshold.load(); }

		/// <summary>
		/// Sets max tilt angle
		/// </summary>
		/// <param name="angle"> Maximal slope the agent can go on </param>
		inline void SetMaxTiltAngle(float angle) { m_agentOptions.angleThreshold = Math::Min(Math::Max(0.0f, angle), 180.0f); }

		/// <summary> Navigation mesh agent flags </summary>
		inline NavMesh::AgentFlags AgentFlags()const { return m_agentOptions.agentFlags.load(); }

		/// <summary>
		/// Sets navigation mesh agent flags
		/// </summary>
		/// <param name="flags"> NavMesh agent flags </param>
		inline void SetFlags(NavMesh::AgentFlags flags) { m_agentOptions.agentFlags = flags; }

		/// <summary> Number of idle frames in-between path recalculations </summary>
		inline uint32_t UpdateInterval()const { return m_updateInterval; }

		/// <summary>
		/// Sets update interval
		/// </summary>
		/// <param name="interval"> Idle frame count </param>
		inline void SetUpdateInterval(uint32_t interval) { m_updateInterval = interval; }

		/// <summary>
		/// Provides the direction to go towards
		/// </summary>
		/// <param name="...args"> Some contextual arguments </param>
		/// <returns> Direction </returns>
		virtual std::optional<Vector3> EvaluateInput()override;

		/// <summary>
		/// Gives access to sub-serializers/fields
		/// </summary>
		/// <param name="recordElement"> Each sub-serializer should be reported by invoking this callback with serializer & corresonding target as parameters </param>
		virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override;

	protected:
		/// <summary> Invoked, when the component becomes active </summary>
		virtual void OnComponentEnabled()override;

		/// <summary> Invoked, when the component becomes inactive </summary>
		virtual void OnComponentDisabled()override;

	private:
		const Reference<NavMesh> m_navMesh;
		const Reference<Object> m_updater;
		WeakReference<InputProvider<Vector3>> m_target;
		WeakReference<InputProvider<Vector3>> m_agentPositionOverride;
		WeakReference<InputProvider<Vector3>> m_agentUpDirectionOverride;
		
		struct AgentOptions {
			std::atomic<float> radius = 1.0f;
			std::atomic<float> surfaceSearchRadius = 1.0f;
			std::atomic<float> angleThreshold = 15.0f;
			std::atomic<NavMesh::AgentFlags> agentFlags = NavMesh::AgentFlags::FIXED_UP_DIRECTION;
			TimelineCurve<float> slopeWeight;

			inline AgentOptions() {}
			inline AgentOptions(const AgentOptions& src) { (*this) = src; }
			inline AgentOptions& operator=(const AgentOptions& src) {
				radius = src.radius.load();
				angleThreshold = src.angleThreshold.load();
				agentFlags = src.agentFlags.load();
				slopeWeight = src.slopeWeight;
				return *this;
			}
		};
		AgentOptions m_agentOptions;
		
		uint32_t m_updateInterval = 8u;
		std::atomic<uint64_t> m_updateFrame = 0u;

		struct AgentState {
			SpinLock positionLock;
			Vector3 position = Vector3(0.0f);
			Vector3 up = Vector3(0.0f);
			std::optional<Vector3> targetPosition;
			SpinLock pathLock;
			std::shared_ptr<std::vector<NavMesh::PathNode>> path = std::make_shared<std::vector<NavMesh::PathNode>>();
		};
		const std::shared_ptr<AgentState> m_state = std::make_shared<AgentState>();

		struct Helpers;
	};

	// Attributes
	template<> inline void TypeIdDetails::GetParentTypesOf<NavMeshAgent>(const Callback<TypeId>& report) { report(TypeId::Of<NavMeshAgent>()); }
	template<> JIMARA_STATE_MACHINES_API void TypeIdDetails::GetTypeAttributesOf<NavMeshAgent>(const Callback<const Object*>& report);
}
