#pragma once
#include <Jimara/Data/Mesh.h>
#include <Jimara/Environment/Scene/Scene.h>
#include <Jimara/Core/Stopwatch.h>


namespace Jimara {
	namespace SampleGame {
		JIMARA_REGISTER_TYPE(Jimara::SampleGame::ObjectEmitter);

		class ObjectEmitter : public virtual Scene::LogicContext::UpdatingComponent {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="parent"> Parent object </param>
			inline ObjectEmitter(Component* parent) : Component(parent, "ObjectEmitter") {}

			inline TriMesh* Mesh()const { return m_mesh; }

			inline void SetMesh(TriMesh* mesh) { m_mesh = mesh; }

			inline float& ColliderRadius() { return m_radius; }

			inline float& EmitterRadius() { return m_radius; }

			inline bool& EnableCCD() { return m_enableCCD; }

			struct Range {
				float minimal = 0.0f;
				float maximal = 1.0f;
			};

			inline Range& Scale() { return m_scaleRange; }

			inline Range& Interval() { return m_intervalRange; }

			inline Range& Lifetime() { return m_lifetimeRange; }

			inline Vector3& Direction() { return m_direction; }

			inline Range& Speed() { return m_speed; }

			inline float& Spread() { return m_spread; }

		protected:
			virtual void Update()override;

		private:
			// Fields
			Reference<TriMesh> m_mesh = nullptr;
			float m_radius = 1.0f;
			float m_emitterRadius = 1.0f;
			bool m_enableCCD = false;
			Range m_scaleRange = { 0.75f, 1.5f };
			Range m_intervalRange = { 0.25f, 0.5f };
			Range m_lifetimeRange = { 0.5f, 1.25f };
			Vector3 m_direction = Math::Up();
			Range m_speed = { 1.5f, 3.0f };
			float m_spread = 30.0f;
			Stopwatch m_stopwatch;
			float m_waitTime = 0.0f;
		};
	}

	// TypeIdDetails::GetTypeAttributesOf exposes the serializer
	template<> void TypeIdDetails::GetTypeAttributesOf<SampleGame::ObjectEmitter>(const Callback<const Object*>& report);
}
