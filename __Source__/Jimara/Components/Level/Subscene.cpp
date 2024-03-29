#include "Subscene.h"


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

		inline static void Synch(Subscene* self) {
			if (self->Destroyed() || self->Context()->Updating()) {
				self->Reload();
				return;
			}
			UpdateSpawnedHierarchy(self, false);
		}

		inline static void OnDestroyed(Subscene* self, Component*) {
			self->SetContent(nullptr);
			self->Reload();
			self->OnDestroyed() -= Callback(Helpers::OnDestroyed, self);
		}

		class SpownedHierarchyRoot : public virtual Component {
		public:
			Subscene* const spawner;

			inline SpownedHierarchyRoot(Subscene* subscene, ComponentHeirarchySpowner* content)
				: Component(subscene->Context(), "Subscene_SpownedHierarchyRoot"), spawner(subscene) {
				content->SpownHeirarchy(Object::Instantiate<Transform>(this, "Subscene_Transform"));
			}

			inline virtual ~SpownedHierarchyRoot() {}
		};
	};

	Subscene::Subscene(Component* parent, const std::string_view& name, ComponentHeirarchySpowner* content) 
		: Component(parent, name) {
		OnDestroyed() += Callback(Helpers::OnDestroyed, this);
		SetContent(content);
	}

	Subscene::~Subscene() {
		OnDestroyed() -= Callback(Helpers::OnDestroyed, this);
		Reload(); 
	}

	ComponentHeirarchySpowner* Subscene::Content()const { return m_content; }

	void Subscene::SetContent(ComponentHeirarchySpowner* content) {
		{
			Subscene* subscene = GetSubscene(this);
			while (subscene != nullptr) {
				if (subscene->m_content == content) {
					Context()->Log()->Error("Subscene::SetContent - Recursive Subscene chain detected! <Component: '", Name(), "'>");
					content = nullptr;
					break;
				}
				else subscene = GetSubscene(subscene);
			}
		}

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
		Context()->Graphics()->PreGraphicsSynch() -= Callback<>(Helpers::Synch, this);

		// Recreate:
		if (m_content == nullptr || Destroyed()) 
			return;
		if (Context()->Updating()) {
			m_spownedHierarchy = Object::Instantiate<Transform>(this, "Subscene_Transform");
			m_content->SpownHeirarchy(m_spownedHierarchy);
		}
		else {
			m_spownedHierarchy = Object::Instantiate<Helpers::SpownedHierarchyRoot>(this, m_content);
			Helpers::UpdateSpawnedHierarchy(this, true);
			Context()->Graphics()->PreGraphicsSynch() += Callback<>(Helpers::Synch, this);
		}
	}

	void Subscene::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		Component::GetFields(recordElement);
		{
			typedef ComponentHeirarchySpowner* (*GetFn)(Subscene*);
			typedef void (*SetFn)(ComponentHeirarchySpowner* const&, Subscene*);
			static const auto serializer = Serialization::ValueSerializer<ComponentHeirarchySpowner*>::Create<Subscene>(
				"Content", "Component hierary to spawn",
				(GetFn)[](Subscene* target) -> ComponentHeirarchySpowner* {
					return target->Content();
				}, (SetFn)[](ComponentHeirarchySpowner* const& value, Subscene* target) {
					if (value != nullptr)
						target->SetContent(nullptr);
					target->SetContent(value);
				});
			recordElement(serializer->Serialize(this));
		}
	}

	AABB Subscene::GetBoundaries()const {
		std::vector<const BoundedObject*> boundedObjects;
		if (m_spownedHierarchy != nullptr) {
			m_spownedHierarchy->GetComponentsInChildren<BoundedObject>(boundedObjects, true);
			const BoundedObject* rootObj = dynamic_cast<const BoundedObject*>(m_spownedHierarchy.operator->());
			if (rootObj != nullptr)
				boundedObjects.push_back(rootObj);
		}
		auto isUnbound = [](const AABB& bnd) {
			return !(
				std::isfinite(bnd.start.x) && std::isfinite(bnd.end.x) &&
				std::isfinite(bnd.start.y) && std::isfinite(bnd.end.y) &&
				std::isfinite(bnd.start.z) && std::isfinite(bnd.end.z));
		};
		AABB bounds = AABB(
			Vector3(std::numeric_limits<float>::quiet_NaN()),
			Vector3(std::numeric_limits<float>::quiet_NaN()));
		for (size_t i = 0u; i < boundedObjects.size(); i++) {
			const AABB bnd = boundedObjects[i]->GetBoundaries();
			if (isUnbound(bnd))
				continue;
			if (isUnbound(bounds))
				bounds = bnd;
			bounds = AABB(
				Vector3(
					Math::Min(bounds.start.x, bnd.start.x, bnd.end.x),
					Math::Min(bounds.start.y, bnd.start.y, bnd.end.y),
					Math::Min(bounds.start.z, bnd.start.z, bnd.end.z)),
				Vector3(
					Math::Max(bounds.end.x, bnd.start.x, bnd.end.x),
					Math::Max(bounds.end.y, bnd.start.y, bnd.end.y),
					Math::Max(bounds.end.z, bnd.start.z, bnd.end.z)));
		}
		return bounds;
	}

	Subscene* Subscene::GetSubscene(Component* instance) {
		if (instance == nullptr) return nullptr;
		while (true) {
			Component* parent = instance->Parent();
			if (parent == nullptr) break;
			instance = parent;
		}
		Helpers::SpownedHierarchyRoot* object = dynamic_cast<Helpers::SpownedHierarchyRoot*>(instance);
		if (object != nullptr) return object->spawner;
		else return nullptr;
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<Subscene>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<Subscene>(
			"Subscene", "Jimara/Level/Subscene", "Subscene spawner");
		report(factory);
	}
}
