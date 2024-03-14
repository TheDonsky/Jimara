#pragma once
#include "NavMesh.h"
#include <Jimara/Core/Systems/InputProvider.h>
#include <Jimara-GenericInputs/Base/VectorInput.h>


namespace Jimara {
	JIMARA_REGISTER_TYPE(Jimara::NavMeshAgent);

	class JIMARA_STATE_MACHINES_API NavMeshAgent 
		: public virtual VectorInput::ComponentFrom<Vector3> {
	public:
		NavMeshAgent(Component* parent, const std::string_view& name = "NavMeshAgent");

		virtual ~NavMeshAgent();

		inline InputProvider<Vector3>* Target()const { 
			return m_target.operator Jimara::Reference<Jimara::InputProvider<Jimara::Vector3>>(); 
		}

		inline void SetTarget(InputProvider<Vector3>* target) { m_target = target; }

		inline const std::vector<NavMesh::PathNode>& Path()const { return m_path; }

		inline float Radius()const { return m_radius; }

		inline void SetRadius(float radius) { m_radius = Math::Max(radius, 0.0f); }

		inline float MaxTiltAngle()const { return m_angleThreshold; }

		inline uint32_t UpdateInterval()const { return m_updateInterval; }

		inline void SetUpdateInterval(uint32_t interval) { m_updateInterval = interval; }

		inline void SetMaxTiltAngle(float angle) { m_angleThreshold = Math::Min(Math::Max(0.0f, angle), 180.0f); }

		void RecalculatePath();

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
		virtual void OnComponentEnabled()override;

		virtual void OnComponentDisabled()override;

	private:
		const Reference<NavMesh> m_navMesh;
		WeakReference<InputProvider<Vector3>> m_target;
		
		float m_radius = 1.0f;
		float m_angleThreshold = 15.0f;
		NavMesh::AgentFlags m_agentFlags = NavMesh::AgentFlags::FIXED_UP_DIRECTION;
		
		uint32_t m_updateInterval = 8u;
		std::vector<NavMesh::PathNode> m_path;
		std::atomic<uint64_t> m_updateFrame = 0u;

		struct Helpers;
	};

	// Attributes
	template<> inline void TypeIdDetails::GetParentTypesOf<NavMeshAgent>(const Callback<TypeId>& report) { report(TypeId::Of<NavMeshAgent>()); }
	template<> JIMARA_STATE_MACHINES_API void TypeIdDetails::GetTypeAttributesOf<NavMeshAgent>(const Callback<const Object*>& report);
}
