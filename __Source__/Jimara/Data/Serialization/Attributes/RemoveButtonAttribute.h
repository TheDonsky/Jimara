#pragma once
#include "../ItemSerializers.h"


namespace Jimara {
	namespace Serialization {
		/// <summary>
		/// Arbitrary action button that appears as a red 'X' inside the editor next to the field, 
		/// implying that clicking it should delete something
		/// </summary>
		class JIMARA_API RemoveButtonAttribute : public virtual Object {
		public:
			/// <summary>
			/// Creates an instance of a RemoveButtonAttribute
			/// </summary>
			/// <typeparam name="TargetType"> Concrete type for reinterpreting Serialization::SerializedObject::TargetAddr inside OnButtonClicked() call </typeparam>
			/// <param name="onClicked"> Event to invoke when the button gets clicked (Serialization::SerializedObject::TargetAddr will be passed as the argument) </param>
			/// <returns> New instance of a RemoveButtonAttribute </returns>
			template<typename TargetType>
			inline static Reference<const Object> Create(const Callback<TargetType*>& onClicked) {
				struct Action : public virtual Object { 
					const Callback<TargetType*> act; 
					inline Action(const Callback<TargetType*>& a) : act(a) {}
					void Perform(const Serialization::SerializedObject& object)const { act((TargetType*)object.TargetAddr()); }
				};
				const Reference<const Action> action = Object::Instantiate<Action>(onClicked);
				const Reference<RemoveButtonAttribute> instance = new RemoveButtonAttribute(
					Callback<const Serialization::SerializedObject&>(&Action::Perform, action.operator->()), action.operator->());
				instance->ReleaseRef();
				return instance;
			}

			/// <summary> Virtual destructor </summary>
			inline virtual ~RemoveButtonAttribute() {}

			/// <summary>
			/// Invoked by the editor when the button gets clicked
			/// </summary>
			/// <param name="object"> Serialized object, this attribute was attached to </param>
			inline void OnButtonClicked(const Serialization::SerializedObject& object)const { m_action(object); }

		private:
			// Internals:
			const Callback<const Serialization::SerializedObject&> m_action;
			const Reference<const Object> m_onClickedHolder;

			// Only concrete Instance can access the constructor
			inline RemoveButtonAttribute(const Callback<const Serialization::SerializedObject&>& action, const Object* onClickedHolder) 
				: m_action(action), m_onClickedHolder(onClickedHolder) {}
		};
	}
}
