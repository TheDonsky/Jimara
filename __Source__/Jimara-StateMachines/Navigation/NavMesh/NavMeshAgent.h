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

		/// <summary> Last path that has been calculated </summary>
		inline const std::vector<NavMesh::PathNode>& Path()const { return m_path; }

		/// <summary> Recalculates path 'here and now' </summary>
		void RecalculatePath();

		/// <summary> Agent radius </summary>
		inline float Radius()const { return m_radius; }

		/// <summary>
		/// Sets agent radius
		/// </summary>
		/// <param name="radius"> Radius to use </param>
		inline void SetRadius(float radius) { m_radius = Math::Max(radius, 0.0f); }

		/// <summary> Maximal slope the agent can go on (if it can climb walls, this angle becomes angle between two surface faces) </summary>
		inline float MaxTiltAngle()const { return m_angleThreshold; }

		/// <summary>
		/// Sets max tilt angle
		/// </summary>
		/// <param name="angle"> Maximal slope the agent can go on </param>
		inline void SetMaxTiltAngle(float angle) { m_angleThreshold = Math::Min(Math::Max(0.0f, angle), 180.0f); }

		/// <summary> Navigation mesh agent flags </summary>
		inline NavMesh::AgentFlags AgentFlags()const { return m_agentFlags; }

		/// <summary>
		/// Sets navigation mesh agent flags
		/// </summary>
		/// <param name="flags"> NavMesh agent flags </param>
		inline void SetFlags(NavMesh::AgentFlags flags) { m_agentFlags = flags; }

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
		
		float m_radius = 1.0f;
		float m_angleThreshold = 15.0f;
		NavMesh::AgentFlags m_agentFlags = NavMesh::AgentFlags::FIXED_UP_DIRECTION;
		TimelineCurve<float> m_slopeWeight;
		
		uint32_t m_updateInterval = 8u;
		std::vector<NavMesh::PathNode> m_path;
		std::atomic<uint64_t> m_updateFrame = 0u;

		struct Helpers;
	};

	// Attributes
	template<> inline void TypeIdDetails::GetParentTypesOf<NavMeshAgent>(const Callback<TypeId>& report) { report(TypeId::Of<NavMeshAgent>()); }
	template<> JIMARA_STATE_MACHINES_API void TypeIdDetails::GetTypeAttributesOf<NavMeshAgent>(const Callback<const Object*>& report);
}
