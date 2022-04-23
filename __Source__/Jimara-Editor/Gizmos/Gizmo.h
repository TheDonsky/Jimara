#pragma once
#include "GizmoScene.h"


namespace Jimara {
	namespace Editor {
		/// <summary>
		/// Gizmos are components that have the duty of displaying various Component handles and icons in a separate scene
		/// Created by a corresponding SceneView
		/// </summary>
		class Gizmo : public virtual Component {
		public:
			/// <summary> These flags define the rules by which the Gizmos should be created and linked to corresponding targets </summary>
			enum class FilterFlag : uint16_t;

			/// <summary> Bitmask of FilterFlags creates a Filter </summary>
			typedef uint16_t Filter;

			/// <summary> Component-to-Gizmo "connection" information </summary>
			class ComponentConnection;

			/// <summary> Set of currently established Component ro Gizmo connections </summary>
			class ComponentConnectionSet;

			/// <summary> Default filter for ComponentConnection </summary>
			inline static constexpr Filter DefaultFilter();

			/// <summary>
			/// Registers Gizmo to Component type connection
			/// <para/> Note: If GizmoType to ComponentType connection pair already exists, the filter will simply be overriden.
			/// </summary>
			/// <param name="connection"> Connection to register </param>
			static void AddConnection(const ComponentConnection& connection);

			/// <summary>
			/// Removes GizmoType to ComponentType connection pair
			/// <para/> Note: FilterFlags are ignored here..
			/// </summary>
			/// <param name="connection"> Connection to remove </param>
			static void RemoveConnection(const ComponentConnection& connection);

			/// <summary> Gizmo context </summary>
			GizmoScene::Context* GizmoContext()const;

			/// <summary> Target component count (Useful only if CREATE_ONE_FOR_ALL_TARGETS is used) </summary>
			inline size_t TargetCount()const { return m_targets.Size(); }

			/// <summary>
			/// Target component by index
			/// </summary>
			/// <param name="index"> Target component index (Useful only if CREATE_ONE_FOR_ALL_TARGETS is used) </param>
			/// <returns> Target component </returns>
			inline Component* TargetComponent(size_t index = 0)const { return m_targets.Size() > index ? m_targets[index] : nullptr; }

			/// <summary>
			/// Target component by index (type-casted to concrete component type)
			/// </summary>
			/// <typeparam name="ComponentType"> Concrete component type </typeparam>
			/// <param name="index"> Target component index (Useful only if CREATE_ONE_FOR_ALL_TARGETS is used) </param>
			/// <returns> Target component </returns>
			template<typename ComponentType>
			inline ComponentType* Target(size_t index = 0)const { return dynamic_cast<ComponentType*>(TargetComponent(index)); }

			/// <summary>
			/// Sets target components
			/// </summary>
			/// <typeparam name="ReferenceType"> Reference<Component>/Reference<ComponentType>/Component*/ComponentType* </typeparam>
			/// <param name="targets"> List of target components (nullptr will clear selection) </param>
			/// <param name="count"> Number of target components </param>
			template<typename ReferenceType>
			inline void SetTarget(const ReferenceType* targets, size_t count = 1) {
				m_targets.Clear();
				if (targets != nullptr)
					for (size_t i = 0; i < count; i++) {
						Component* target = targets[i];
						if (target != nullptr)
							m_targets.Push(target);
					}
			}

		private:
			// Target components
			Stacktor<Reference<Component>, 1> m_targets;

			// Context
			mutable Reference<GizmoScene::Context> m_context;
		};

		/// <summary>
		/// These flags define the rules by which the Gizmos should be created and linked to corresponding targets
		/// </summary>
		enum class Gizmo::FilterFlag : uint16_t {
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


		/// <summary> Component-to-Gizmo "connection" information </summary>
		class Gizmo::ComponentConnection {
		public:
			/// <summary> Default constructor </summary>
			inline ComponentConnection() {}

			/// <summary>
			/// Constructor
			/// </summary>
			/// <typeparam name="GizmoType"> Type of the gizmo </typeparam>
			/// <typeparam name="ComponentType"> Type of the component, targetted by the gizmo </typeparam>
			/// <param name="filter"> Gizmo filter flags </param>
			/// <returns> ComponentConnection </returns>
			template<typename GizmoType, typename ComponentType>
			inline static constexpr std::enable_if_t<std::is_base_of_v<Component, ComponentType>, ComponentConnection> Make(Filter filter = DefaultFilter()) {
				return ComponentConnection(TypeId::Of<GizmoType>(), TypeId::Of<ComponentType>(), filter, Object::Instantiate<Gizmo, Scene::LogicContext*>);
			}

			/// <summary>
			/// Constructor
			/// </summary>
			/// <typeparam name="GizmoType"> Type of the gizmo </typeparam>
			/// <typeparam name="ComponentType"> Type of the component, targetted by the gizmo </typeparam>
			/// <param name="flag"> Filter flag </param>
			/// <returns> ComponentConnection </returns>
			template<typename GizmoType, typename ComponentType>
			inline static constexpr std::enable_if_t<std::is_base_of_v<Component, ComponentType>, ComponentConnection> Make(FilterFlag flag) {
				return Make<Component, ComponentType>(static_cast<Filter>(flag));
			}

			/// <summary>
			/// Component Connection with void target type
			/// </summary>
			/// <typeparam name="GizmoType"> Type of the gizmo </typeparam>
			/// <returns> ComponentConnection with CREATE_WITHOUT_TARGET flag and no target </returns>
			template<typename GizmoType>
			inline static constexpr ComponentConnection Targetless() {
				return ComponentConnection(TypeId::Of<GizmoType>(), TypeId::Of<void>(),
					static_cast<Filter>(FilterFlag::CREATE_WITHOUT_TARGET), Object::Instantiate<Gizmo, Scene::LogicContext*>);
			}

			/// <summary> Type of the gizmo </summary>
			inline TypeId GizmoType()const { return m_gizmoType; }

			/// <summary> Type of the component, targetted by the gizmo </summary>
			inline TypeId ComponentType()const { return m_componentType; }

			/// <summary> Gizmo filter flags </summary>
			inline Filter FilterFlags()const { return m_filter; }

			/// <summary>
			/// Creates a gizmo
			/// </summary>
			/// <param name="gizmoSceneContext"> "Gizmo-Scene" logic context </param>
			/// <returns> Newly created gizmo </returns>
			inline Reference<Gizmo> CreateGizmo(Scene::LogicContext* gizmoSceneContext) { return m_createGizmo(gizmoSceneContext); }

		private:
			// Type of the gizmo
			TypeId m_gizmoType = TypeId::Of<void>();

			// Type of the component, targetted by the gizmo
			TypeId m_componentType = TypeId::Of<void>();

			// Filter
			Filter m_filter = 0;
			
			// Create function
			typedef Reference<Gizmo>(*CreateFn)(Scene::LogicContext*);
			CreateFn m_createGizmo = [](Scene::LogicContext*) -> Reference<Gizmo> { return nullptr; };

			// Constructor
			inline ComponentConnection(TypeId gizmoType, TypeId componentType, Filter filter, CreateFn createGizmo)
				: m_gizmoType(gizmoType), m_componentType(componentType), m_filter(filter), m_createGizmo(createGizmo) {}
		};

		/// <summary> Set of currently established Component ro Gizmo connections </summary>
		class Gizmo::ComponentConnectionSet : public virtual Object {
		public:
			/// <summary> 
			/// Set of all currently existing Component connections 
			/// <para/> Note: The pointer will change whenever anything new gets registered or unregistered. 
			///		Otherwise, the collection will stay immutable.
			/// </summary>
			static Reference<const ComponentConnectionSet> Current();

			/// <summary> List of connections </summary>
			typedef Stacktor<ComponentConnection, 1> ConnectionList;

			/// <summary>
			/// Finds registered gizmo connections for given Component type
			/// </summary>
			/// <param name="componentType"> Component type </param>
			/// <returns> List of Component to Gizmo Connections </returns>
			const ConnectionList& GetGizmosFor(std::type_index componentType)const;

			/// <summary>
			/// Finds registered gizmo connections for given Component type
			/// </summary>
			/// <param name="componentType"> Component type </param>
			/// <returns> List of Component to Gizmo Connections </returns>
			inline const ConnectionList& GetGizmosFor(TypeId componentType)const { return GetGizmosFor(componentType.TypeIndex()); }

			/// <summary>
			/// Finds registered gizmo connections for given Component
			/// </summary>
			/// <param name="component"> Component </param>
			/// <returns> List of Component to Gizmo Connections </returns>
			inline const ConnectionList& GetGizmosFor(Component* component)const { return GetGizmosFor(typeid(*component)); }

			/// <summary> Retrieves list of all gizmo connections, that are registered for void or with CREATE_WITHOUT_TARGET flag </summary>
			inline const ConnectionList& GetTargetlessGizmos()const { return m_targetlessGizmos; }

		private:
			// Connections per component type
			std::unordered_map<std::type_index, ConnectionList> m_connections;

			// All connections with CREATE_WITHOUT_TARGET flag
			ConnectionList m_targetlessGizmos;
		};

		/// <summary> Logical 'or' between two Gizmo::FilterFlag-s </summary>
		inline constexpr Gizmo::Filter operator|(const Gizmo::FilterFlag& a, const Gizmo::FilterFlag& b) { return static_cast<Gizmo::Filter>(a) | static_cast<Gizmo::Filter>(b); }

		/// <summary> Logical 'or' between Gizmo::Filter and Gizmo::FilterFlag </summary>
		inline constexpr Gizmo::Filter operator|(const Gizmo::Filter& a, const Gizmo::FilterFlag& b) { return a | static_cast<Gizmo::Filter>(b); }

		/// <summary> Logical 'or' between Gizmo::FilterFlag and Gizmo::Filter </summary>
		inline constexpr Gizmo::Filter operator|(const Gizmo::FilterFlag& a, const Gizmo::Filter& b) { return static_cast<Gizmo::Filter>(a) | b; }

		/// <summary> Logical 'and' between two Gizmo::FilterFlag-s </summary>
		inline constexpr Gizmo::Filter operator&(const Gizmo::FilterFlag& a, const Gizmo::FilterFlag& b) { return static_cast<Gizmo::Filter>(a) & static_cast<Gizmo::Filter>(b); }

		/// <summary> Logical 'and' between Gizmo::Filter and Gizmo::FilterFlag </summary>
		inline constexpr Gizmo::Filter operator&(const Gizmo::Filter& a, const Gizmo::FilterFlag& b) { return a & static_cast<Gizmo::Filter>(b); }

		/// <summary> Logical 'and' between Gizmo::FilterFlag and Gizmo::Filter </summary>
		inline constexpr Gizmo::Filter operator&(const Gizmo::FilterFlag& a, const Gizmo::Filter& b) { return static_cast<Gizmo::Filter>(a) & b; }

		/// <summary> Compares Gizmo::Filter to Gizmo::FilterFlag </summary>
		inline constexpr bool operator==(const Gizmo::Filter& a, const Gizmo::FilterFlag& b) { return a == static_cast<Gizmo::Filter>(b); }

		/// <summary> Compares Gizmo::FilterFlag to Gizmo::Filter </summary>
		inline constexpr bool operator==(const Gizmo::FilterFlag& a, const Gizmo::Filter& b) { return static_cast<Gizmo::Filter>(a) == b; }

		/// <summary> Compares Gizmo::Filter to Gizmo::FilterFlag </summary>
		inline constexpr bool operator!=(const Gizmo::Filter& a, const Gizmo::FilterFlag& b) { return a != static_cast<Gizmo::Filter>(b); }

		/// <summary> Compares Gizmo::FilterFlag to Gizmo::Filter </summary>
		inline constexpr bool operator!=(const Gizmo::FilterFlag& a, const Gizmo::Filter& b) { return static_cast<Gizmo::Filter>(a) != b; }

		/// <summary> Default filter for ComponentConnection </summary>
		inline constexpr Gizmo::Filter Gizmo::DefaultFilter() {
			return
				FilterFlag::CREATE_IF_SELECTED |				// If we select the target, there should be a gizmo
				FilterFlag::CREATE_IF_PARENT_SELECTED |			// If parent gets selected, there should be a gizmo
				FilterFlag::CREATE_CHILD_GIZMOS_IF_SELECTED |	// If any of the children wants to draw a gizmo, why not
				FilterFlag::CREATE_PARENT_GIZMOS_IF_SELECTED;	// If any of the parents wants to draw it's gizmo, why not

			// Note: For Transform, we could go like this:
			// CREATE_IF_SELECTED | CREATE_IF_CHILD_SELECTED | CREATE_CHILD_GIZMOS_IF_SELECTED | CREATE_ONE_FOR_ALL_TARGETS
		}

		// A bunch of compilation-time sanity checks
		static_assert(static_cast<Gizmo::Filter>(Gizmo::FilterFlag::CREATE_IF_SELECTED) == 1);
		static_assert((Gizmo::FilterFlag::CREATE_IF_SELECTED | Gizmo::FilterFlag::CREATE_IF_NOT_SELECTED) == 3);
		static_assert(((Gizmo::FilterFlag::CREATE_IF_SELECTED | Gizmo::FilterFlag::CREATE_IF_NOT_SELECTED) | Gizmo::FilterFlag::CREATE_IF_PARENT_SELECTED) == 7);
		static_assert((Gizmo::FilterFlag::CREATE_IF_SELECTED | (Gizmo::FilterFlag::CREATE_IF_NOT_SELECTED | Gizmo::FilterFlag::CREATE_IF_PARENT_SELECTED)) == 7);
		static_assert((Gizmo::FilterFlag::CREATE_IF_SELECTED& Gizmo::FilterFlag::CREATE_IF_NOT_SELECTED) == 0);
		static_assert(((Gizmo::FilterFlag::CREATE_IF_SELECTED& Gizmo::FilterFlag::CREATE_IF_SELECTED) | Gizmo::FilterFlag::CREATE_IF_NOT_SELECTED) == 3);
		static_assert((Gizmo::FilterFlag::CREATE_IF_SELECTED& (Gizmo::FilterFlag::CREATE_IF_SELECTED | Gizmo::FilterFlag::CREATE_IF_NOT_SELECTED)) == Gizmo::FilterFlag::CREATE_IF_SELECTED);
	}
}
