#pragma once
#include "../Environment/EditorScene.h"


namespace Jimara {
	namespace Editor {
		/// <summary>
		/// TODO: Document this crap!
		/// </summary>
		class Gizmo : public virtual Component {
		public:
			/// <summary>
			/// These flags define the rules by which the Gizmos should be created and linked to corresponding targets
			/// </summary>
			enum class FilterFlag : uint16_t {
				/// <summary> Will create gizmo if target is selected </summary>
				CREATE_IF_SELECTED = (1 << 0),

				/// <summary> 
				/// Will create gizmo if target is not selected 
				/// <para/> Note: If CREATE_IF_SELECTED flag is not set and 
				///		neither one of the parent-child relationship flags cause the Gizmo to appear, 
				///		gizmo will be destroyed on selection.
				/// </summary>
				CREATE_IF_NOT_SELECTED = (1 << 1),

				/// <summary> 
				/// Will create gizmo if any component from target's parent chain is selected
				/// <para/> Note: Abscence of CREATE_CHILD_GIZMOS_IF_SELECTED flag in any of the parent componentss' gizmos 
				///		leading up to the selected ones will override behaviour and prevent gizmo creation.
				/// </summary>
				CREATE_IF_PARENT_SELECTED = (1 << 2),

				/// <summary>
				/// Will create gizmo if any object within the child subtree is selected
				/// <para/> Note: Abscence of CREATE_PARENT_GIZMOS_IF_SELECTED in any component's parent chain 
				///		leading up to the target will override behaviour and prevent gizmo creation. 
				///		However, if multiple items are selected and at least any parent chain
				///		is connected with components that all have CREATE_PARENT_GIZMOS_IF_SELECTED flags set or do not have 
				///		any gizmos, the target gizmo will be created as expected.
				/// </summary>
				CREATE_IF_CHILD_SELECTED = (1 << 3),

				/// <summary>
				/// If target is selected, child component gizmos will be created
				/// <apara/> Note: Works in combination of CREATE_IF_PARENT_SELECTED and does not have recursive effect, 
				///		with an exception of the case, when child component has no gizmos.
				///		In the latter case 'grandchildren gizmos' will be effected recursively, till we meet some component that has registered gizmos
				/// </summary>
				CREATE_CHILD_GIZMOS_IF_SELECTED = (1 << 4),

				/// <summary>
				/// If target is selected, parent component gizmos will be created
				/// <apara/> Note: Works in combination with CREATE_IF_CHILD_SELECTED and does not have recursive effect, 
				///		with an exception of the case, when parent component has no gizmos.
				/// </summary>
				CREATE_PARENT_GIZMOS_IF_SELECTED = (1 << 5),

				/// <summary> If set, this flag will cause a single unified gizmo to be created for the entire selection effected by it </summary>
				CREATE_ONE_FOR_ALL_TARGETS = (1 << 6),

				/// <summary> 
				/// If set, there will always be a single gizmo instance present without any targets
				/// <para/> Note: Useful for general navigation, on-screen selection and similar stuff.
				/// </summary>
				CREATE_WITHOUT_TARGET = (1 << 7)
			};

			typedef uint16_t Filter;

			virtual TypeId TargetType()const { return TypeId::Of<Component>(); }

			inline size_t TargetCount()const { return m_targets.Size(); }

			inline Component* TargetComponent(size_t index = 0)const { return m_targets.Size() > index ? m_targets[index] : nullptr; }

			template<typename ReferenceType>
			inline void SetTarget(const ReferenceType* targets, size_t count = 0) {
				m_targets.Clear();
				TypeId targetType = TargetType();
				if (targets != nullptr)
					for (size_t i = 0; i < count; i++) {
						Component* target = targets[i];
						if (targetType.CheckType(target))
							m_targets.Push(target);
					}
			}

			inline static constexpr Filter DefaultFilter();

			template<typename GizmoType>
			inline static void Register(TypeId targetComponentType, Filter filter = DefaultFilter()) {
				RegisterGizmo(
					TypeId::Of<GizmoType>(), Object::Instantiate<Gizmo, Scene::LogicContext*>,
					targetComponentType, filter);
			}

			template<typename GizmoType>
			inline static void Unregister(TypeId targetComponentType) {
				UnregisterGizmo(TypeId::Of<GizmoType>(), targetComponentType);
			}

			template<typename TargetComponentType>
			class Of;

		private:
			Stacktor<Reference<Component>, 1> m_targets;

			static void RegisterGizmo(TypeId gizmoType, TypeId targetComponentType, Reference<Gizmo>(*create)(Scene::LogicContext*), Filter filter);
			static void UnregisterGizmo(TypeId gizmoType, TypeId targetComponentType);
		};

		template<typename TargetComponentType>
		class Gizmo::Of : public virtual Gizmo {
		public:
			virtual TypeId TargetType()const final override { return TypeId::Of<TargetComponentType>(); }

			inline TargetComponentType* Target(size_t index = 0)const { return dynamic_cast<TargetComponentType*>(TargetComponent(index)); }
		};


		inline constexpr Gizmo::Filter operator|(const Gizmo::FilterFlag& a, const Gizmo::FilterFlag& b) { return static_cast<Gizmo::Filter>(a) | static_cast<Gizmo::Filter>(b); }
		inline constexpr Gizmo::Filter operator|(const Gizmo::Filter& a, const Gizmo::FilterFlag& b) { return a | static_cast<Gizmo::Filter>(b); }
		inline constexpr Gizmo::Filter operator|(const Gizmo::FilterFlag& a, const Gizmo::Filter& b) { return static_cast<Gizmo::Filter>(a) | b; }

		inline constexpr Gizmo::Filter operator&(const Gizmo::FilterFlag& a, const Gizmo::FilterFlag& b) { return static_cast<Gizmo::Filter>(a) & static_cast<Gizmo::Filter>(b); }
		inline constexpr Gizmo::Filter operator&(const Gizmo::Filter& a, const Gizmo::FilterFlag& b) { return a & static_cast<Gizmo::Filter>(b); }
		inline constexpr Gizmo::Filter operator&(const Gizmo::FilterFlag& a, const Gizmo::Filter& b) { return static_cast<Gizmo::Filter>(a) & b; }

		inline constexpr bool operator==(const Gizmo::Filter& a, const Gizmo::FilterFlag& b) { return a == static_cast<Gizmo::Filter>(b); }
		inline constexpr bool operator==(const Gizmo::FilterFlag& a, const Gizmo::Filter& b) { return static_cast<Gizmo::Filter>(a) == b; }

		inline constexpr Gizmo::Filter Gizmo::DefaultFilter() {
			return
				FilterFlag::CREATE_IF_SELECTED |				// If we select the target, there should be a gizmo
				FilterFlag::CREATE_IF_PARENT_SELECTED |			// If parent gets selected, there should be a gizmo
				FilterFlag::CREATE_CHILD_GIZMOS_IF_SELECTED |	// If any of the children wants to draw a gizmo, why not
				FilterFlag::CREATE_PARENT_GIZMOS_IF_SELECTED;	// If any of the parents wants to draw it's gizmo, why not

			// Note: For Transform, we could go like this:
			// CREATE_IF_SELECTED | CREATE_IF_CHILD_SELECTED | CREATE_CHILD_GIZMOS_IF_SELECTED | CREATE_ONE_FOR_ALL_TARGETS
		}

		static_assert(static_cast<Gizmo::Filter>(Gizmo::FilterFlag::CREATE_IF_SELECTED) == 1);
		static_assert((Gizmo::FilterFlag::CREATE_IF_SELECTED | Gizmo::FilterFlag::CREATE_IF_NOT_SELECTED) == 3);
		static_assert(((Gizmo::FilterFlag::CREATE_IF_SELECTED | Gizmo::FilterFlag::CREATE_IF_NOT_SELECTED) | Gizmo::FilterFlag::CREATE_IF_PARENT_SELECTED) == 7);
		static_assert((Gizmo::FilterFlag::CREATE_IF_SELECTED | (Gizmo::FilterFlag::CREATE_IF_NOT_SELECTED | Gizmo::FilterFlag::CREATE_IF_PARENT_SELECTED)) == 7);
		static_assert((Gizmo::FilterFlag::CREATE_IF_SELECTED & Gizmo::FilterFlag::CREATE_IF_NOT_SELECTED) == 0);
		static_assert(((Gizmo::FilterFlag::CREATE_IF_SELECTED & Gizmo::FilterFlag::CREATE_IF_SELECTED) | Gizmo::FilterFlag::CREATE_IF_NOT_SELECTED) == 3);
		static_assert((Gizmo::FilterFlag::CREATE_IF_SELECTED & (Gizmo::FilterFlag::CREATE_IF_SELECTED | Gizmo::FilterFlag::CREATE_IF_NOT_SELECTED)) == Gizmo::FilterFlag::CREATE_IF_SELECTED);
	}
}
