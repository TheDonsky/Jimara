#include "ObjectEmitter.h"
#include <Jimara/Components/Physics/SphereCollider.h>
#include <Jimara/Components/GraphicsObjects/MeshRenderer.h>
#include <Jimara/Math/Random.h>
#include <Jimara/Data/Generators/MeshConstants.h>
#include <Jimara/Data/Materials/SampleDiffuse/SampleDiffuseShader.h>
#include <Jimara/Data/Serialization/Attributes/SliderAttribute.h>
#include <Jimara/Data/Serialization/Helpers/SerializerMacros.h>

namespace Jimara {
	namespace SampleGame {
		namespace {
			class ObjectEmitter_RangeSerializer : public virtual Serialization::SerializerList::From<ObjectEmitter::Range> {
			public:
				inline ObjectEmitter_RangeSerializer(const std::string_view& name, const std::string_view& hint) 
					: Serialization::ItemSerializer(name, hint) {}

				virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, ObjectEmitter::Range* target)const {
					{
						static const auto serializer = Serialization::FloatSerializer::For<ObjectEmitter::Range>(
							"Min", "Minimal value",
							[](ObjectEmitter::Range* target) { return target->minimal; },
							[](const float& value, ObjectEmitter::Range* target) { target->minimal = value; });
						recordElement(serializer->Serialize(target));
					}
					{
						static const auto serializer = Serialization::FloatSerializer::For<ObjectEmitter::Range>(
							"Max", "Maximal value",
							[](ObjectEmitter::Range* target) { return target->maximal; },
							[](const float& value, ObjectEmitter::Range* target) { target->maximal = Math::Max(target->minimal, value); });
						recordElement(serializer->Serialize(target));
					}
				}
			};

#pragma warning(disable: 4250)
			class ObjectEmitter_EmittedObject : public virtual Transform, public virtual SceneContext::UpdatingComponent {
			private:
				const float m_timeout;
				Stopwatch m_stopwatch;

			public:
				inline static float GetRandomValue(const ObjectEmitter::Range& range) {
					return std::uniform_real_distribution<float>(range.minimal, range.maximal)(Random::ThreadRNG());
				};

				inline ObjectEmitter_EmittedObject(ObjectEmitter* emitter)
					: Component(emitter, "Emission"), Transform(emitter, "Emission")
					, m_timeout(GetRandomValue(emitter->Lifetime())) {
					// Transform:
					SetLocalScale(Vector3(GetRandomValue(emitter->Scale())));
					SetLocalPosition(
						GetRandomValue({ 0.0f, emitter->EmitterRadius() }) *
						Math::Normalize(Vector3(
							GetRandomValue({ -1.0f, 1.0f }),
							GetRandomValue({ -1.0f, 1.0f }),
							GetRandomValue({ -1.0f, 1.0f })
						)));
					SetLocalEulerAngles(
						Vector3(
							GetRandomValue({ 0.0f, 1.0f }),
							GetRandomValue({ 0.0f, 1.0f }),
							GetRandomValue({ 0.0f, 1.0f })) * 360.0f);
					
					// Renderer:
					{
						const constexpr Vector3 color(0.0f, 1.0f, 0.0f);
						const Reference<Material::Instance> material = SampleDiffuseShader::MaterialInstance(Context()->Graphics()->Device(), color);
						Object::Instantiate<MeshRenderer>(this, "Renderer", [&]() -> Reference<TriMesh> {
							Reference<TriMesh> mesh = emitter->Mesh();
							if (mesh != nullptr) return mesh;
							else return MeshConstants::Tri::Sphere();
							}())->SetMaterialInstance(material);
					}

					// Physics:
					Rigidbody*const body = Object::Instantiate<Rigidbody>(this);
					Object::Instantiate<SphereCollider>(body, "Collider", emitter->ColliderRadius());
					body->EnableCCD(emitter->EnableCCD());
					const Vector3 direction = [&]() -> Vector3 {
						const Vector3 baseDir = Math::Normalize(emitter->Direction());
						const Vector3 perpA = [&]() -> Vector3 {
							auto tryDir = [&](const Vector3& dir) {
								const Vector3 cross = Math::Cross(dir, baseDir);
								const float sqrMagn = Math::SqrMagnitude(cross);
								if (sqrMagn > std::numeric_limits<float>::epsilon())
									return std::make_pair(true, cross / std::sqrt(sqrMagn));
								else return std::make_pair(false, Vector3(0.0f));
							};
							const auto verA = tryDir(Math::Right());
							if (verA.first) return verA.second;
							else return tryDir(Math::Forward()).second;
						}();
						const Vector3 perpB = Math::Normalize(Math::Cross(baseDir, perpA));
						const float spreadAngle = Math::Radians(GetRandomValue({ -std::abs(emitter->Spread()), std::abs(emitter->Spread()) }));
						const float roundAngle = Math::Radians(GetRandomValue({ 0.0f, 360.0f }));
						return Math::Normalize(
							(baseDir * std::cos(spreadAngle)) +
							(((perpA * std::sin(roundAngle) + perpB * std::cos(roundAngle))) * std::sin(spreadAngle)));
					}();
					body->SetVelocity(direction * GetRandomValue(emitter->Speed()));
				}

			protected:
				inline virtual void Update()override {
					if (m_stopwatch.Elapsed() >= m_timeout) Destroy();
				}
			};
#pragma warning(default: 4250)
		}

		void ObjectEmitter::GetFields(Callback<Serialization::SerializedObject> recordElement) {
			Component::GetFields(recordElement);
			JIMARA_SERIALIZE_FIELDS(this, recordElement) {
				JIMARA_SERIALIZE_FIELD_GET_SET(Mesh, SetMesh, "Mesh", "Shape of the emitted objects");
				JIMARA_SERIALIZE_FIELD(ColliderRadius(), "ColliderRadius", "Collider Radius");
				JIMARA_SERIALIZE_FIELD(EmitterRadius(), "EmitterRadius", "Emission sphere radius");
				JIMARA_SERIALIZE_FIELD(EnableCCD(), "Enable CCD", "Enable/Disable Continuous collision detection on spawned bodies");
				JIMARA_SERIALIZE_FIELD_CUSTOM(Scale(), ObjectEmitter_RangeSerializer, "Scale", "Scale range");
				JIMARA_SERIALIZE_FIELD_CUSTOM(Interval(), ObjectEmitter_RangeSerializer, "Interval", "Emission interval range");
				JIMARA_SERIALIZE_FIELD_CUSTOM(Lifetime(), ObjectEmitter_RangeSerializer, "Lifetime", "Emitted object lifetime");
				JIMARA_SERIALIZE_FIELD(m_direction, "Direction", "Emission cone direction");
				JIMARA_SERIALIZE_FIELD_CUSTOM(Speed(), ObjectEmitter_RangeSerializer, "Speed", "Range of absolute velocity of the emission");
				JIMARA_SERIALIZE_FIELD(m_size, "Spread", "Emission cone angle", Object::Instantiate<Serialization::SliderAttribute<float>>(0.0f, 180.0f));
			};
		}

		void ObjectEmitter::Update() {
			if (m_stopwatch.Elapsed() < m_waitTime) return;
			m_stopwatch.Reset();
			m_waitTime = ObjectEmitter_EmittedObject::GetRandomValue(Interval());
			Object::Instantiate<ObjectEmitter_EmittedObject>(this);
		}
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<SampleGame::ObjectEmitter>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<SampleGame::ObjectEmitter> serializer("SampleGame/ObjectEmitter", "Sample object emitter thing");
		report(&serializer);
	}
}
