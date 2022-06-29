#include "Subscene.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"


namespace Jimara {
	struct Subscene::Helpers {
		inline static bool UpdateTransforms(Subscene* self) {
			Transform* transform = self->GetTransfrom();
			Vector3 position, rotation, scale;
			if (transform != nullptr) {
				position = transform->WorldPosition();
				rotation = transform->WorldEulerAngles();
				scale = transform->LossyScale();
			}
			else {
				position = Vector3(0.0f);
				rotation = Vector3(0.0f);
				scale = Vector3(1.0f);
			}
			bool rv = false;
			auto update = [&](Vector3& value, const Vector3& newValue) {
				if (value == newValue) return;
				value = newValue;
				rv = true;
			};
			update(self->m_lastPosition, position);
			update(self->m_lastEulerAngles, rotation);
			update(self->m_lastScale, scale);
			return rv;
		}

		inline static void UpdateSpawnedHierarchy(Subscene* self, bool forceUpdate) {
			if (self->m_spownedHierarchy == nullptr) return;
			Transform* childTransform = self->m_spownedHierarchy->GetComponentInChildren<Transform>(false); // We have just one child, so this is OK
			if (childTransform == nullptr) return;
			childTransform->SetEnabled(self->ActiveInHeirarchy());
			if ((!UpdateTransforms(self)))
				if (!forceUpdate) return;
			childTransform->SetLocalPosition(self->m_lastPosition);
			childTransform->SetLocalEulerAngles(self->m_lastEulerAngles);
			childTransform->SetLocalScale(self->m_lastScale);
		}

		inline static void Update(Object* selfPtr) {
			Subscene* self = dynamic_cast<Subscene*>(selfPtr);
			if (self->Destroyed()) self->Reload();
			UpdateSpawnedHierarchy(self, false); 
			self->Context()->ExecuteAfterUpdate(Helpers::Update, selfPtr);
		}

		inline static void OnDestroyed(Subscene* self, Component*) {
			self->SetContent(nullptr);
			self->Reload();
			self->OnDestroyed() -= Callback(Helpers::OnDestroyed, self);
		}

		class SpownedHierarchyRoot : public virtual Component {
		public:
			inline SpownedHierarchyRoot(SceneContext* context, ComponentHeirarchySpowner* content) 
				: Component(context, "Subscene_SpownedHierarchyRoot") {
				content->SpownHeirarchy(Object::Instantiate<Transform>(this, "Subscene_Transform"));
			}

			inline virtual ~SpownedHierarchyRoot() {}
		};
	};

	Subscene::Subscene(Component* parent, const std::string_view& name, ComponentHeirarchySpowner* content) 
		: Component(parent, name) {
		OnDestroyed() += Callback(Helpers::OnDestroyed, this);
		Context()->ExecuteAfterUpdate(Helpers::Update, this);
		SetContent(content);
	}

	Subscene::~Subscene() {
		OnDestroyed() -= Callback(Helpers::OnDestroyed, this);
		Reload(); 
	}

	ComponentHeirarchySpowner* Subscene::Content()const { return m_content; }

	void Subscene::SetContent(ComponentHeirarchySpowner* content) {
		if (m_content == content) return;
		else if (Destroyed()) {
			content = nullptr;
			if (m_content == content) return;
		}
		m_content = content;
		Reload();
	}

	void Subscene::Reload() {
		std::unique_lock<std::recursive_mutex> lock(Context()->UpdateLock());

		// Unload:
		if (m_spownedHierarchy != nullptr) {
			if (!m_spownedHierarchy->Destroyed())
				m_spownedHierarchy->Destroy();
			m_spownedHierarchy = nullptr;
		}

		// Recreate:
		if (m_content == nullptr || Destroyed()) return;
		m_spownedHierarchy = Object::Instantiate<Helpers::SpownedHierarchyRoot>(Context(), m_content);
		Helpers::UpdateSpawnedHierarchy(this, true);
	}

	void Subscene::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		Component::GetFields(recordElement);
		JIMARA_SERIALIZE_FIELDS(this, recordElement, {
			JIMARA_SERIALIZE_FIELD_GET_SET(Content, SetContent, "Content", "Component hierary to spawn");
			});
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<Subscene>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<Subscene> serializer("Jimara/Level/Subscene", "Subscene spawner");
		report(&serializer);
	}
}
