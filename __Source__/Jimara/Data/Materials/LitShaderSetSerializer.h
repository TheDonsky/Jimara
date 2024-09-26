#pragma once
#include "Material.h"


namespace Jimara {
	/// <summary>
	/// Serializer for loading/storing lit-shader set records; 
	/// <para/> Compatible with jimara_build_shaders.py output's "LitShaders" node.
	/// </summary>
	class JIMARA_API LitShaderSetSerializer : public virtual Serialization::SerializerList::From<Reference<const Material::LitShaderSet>> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="name"> Name of the ItemSerializer </param>
		/// <param name="hint"> Target hint (editor helper texts on hover and what not) </param>
		/// <param name="attributes"> Serializer attributes </param>
		LitShaderSetSerializer(const std::string_view& name, const std::string_view& hint, const std::vector<Reference<const Object>>& attributes = {});

		/// <summary> Virtual destructor </summary>
		virtual ~LitShaderSetSerializer();

		/// <summary>
		/// Gives access to sub-serializers/fields
		/// </summary>
		/// <param name="recordElement"> Each sub-serializer will be reported by invoking this callback with serializer & corresonding target as parameters </param>
		/// <param name="targetAddr"> Serializer target object </param>
		virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, Reference<const Material::LitShaderSet>* target)const override;
	};
}
