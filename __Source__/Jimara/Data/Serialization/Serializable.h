#pragma once
#include "ItemSerializers.h"
#include "../../Core/Helpers.h"


namespace Jimara {
	namespace Serialization {
		/// <summary>
		/// A class, capable of serializing itself
		/// </summary>
		class JIMARA_API Serializable {
		public:
			/// <summary> Virtual destructor </summary>
			inline virtual ~Serializable() {}

			/// <summary>
			/// Gives access to sub-serializers/fields
			/// </summary>
			/// <param name="recordElement"> Each sub-serializer should be reported by invoking this callback with serializer & corresonding target as parameters </param>
			virtual void GetFields(Callback<SerializedObject> recordElement) { Unused(recordElement); }

			/// <summary>
			/// Simple implementation of a serializer of any Serializable type
			/// </summary>
			class JIMARA_API Serializer : public virtual SerializerList::From<Serializable> {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="name"> Name of the ItemSerializer </param>
				/// <param name="hint"> Target hint (editor helper texts on hover and what not) </param>
				/// <param name="attributes"> Serializer attributes </param>
				inline Serializer(const std::string_view& name, const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {})
					: ItemSerializer(name, hint, attributes) {}

				/// <summary>
				/// Gives access to sub-serializers/fields
				/// </summary>
				/// <param name="recordElement"> Each sub-serializer will be reported by invoking this callback with serializer & corresonding target as parameters </param>
				/// <param name="target"> Serializer target object address </param>
				inline virtual void GetFields(const Callback<SerializedObject>& recordElement, Serializable* target)const override {
					target->GetFields(recordElement);
				}
			};
		};
	}
}
