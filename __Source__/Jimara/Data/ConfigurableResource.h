#pragma once
#include "../Core/TypeRegistration/ObjectFactory.h"
#include "../Graphics/GraphicsDevice.h"
#include "../Data/ShaderLibrary.h"
#include "../Physics/PhysicsInstance.h"
#include "../Audio/AudioDevice.h"
#include "AssetDatabase/AssetDatabase.h"
#include "Serialization/Serializable.h"


namespace Jimara {
	/// <summary> Arbitrary resource that can store some settings and/or configuration </summary>
	class JIMARA_API ConfigurableResource : public virtual Resource, public virtual Serialization::Serializable {
	public:
		/// <summary>
		/// Creation arguments for configurable resources
		/// </summary>
		struct JIMARA_API CreateArgs {
			/// <summary> Logger </summary>
			OS::Logger* log = nullptr;

			/// <summary> Graphics device </summary>
			Graphics::GraphicsDevice* graphicsDevice = nullptr;

			/// <summary> Shader library </summary>
			ShaderLibrary* shaderLibrary = nullptr;

			/// <summary> Physics API instance </summary>
			Physics::PhysicsInstance* physicsInstance = nullptr;

			/// <summary> Audio device </summary>
			Audio::AudioDevice* audioDevice = nullptr;
		};

		/// <summary> Factory for creating configurable resources </summary>
		using ResourceFactory = ObjectFactory<ConfigurableResource, CreateArgs>;

		/// <summary>
		/// Instance reference, alongside CreateArgs for serialization with optional recreation.
		/// <para/> This mainly exists for serializing with InstanceSerializer.
		/// </summary>
		struct JIMARA_API SerializableInstance {
			/// <summary> Instance </summary>
			Reference<ConfigurableResource> instance;

			/// <summary> Create arguments in case there's a need to recreate the resource </summary>
			CreateArgs recreateArgs;
		};

		/// <summary> Serializer, that serializes ConfigurableResource reference and gives a choice of the type as well </summary>
		struct JIMARA_API InstanceSerializer : public virtual Serialization::SerializerList::From<SerializableInstance> {
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="name"> Field/Serializer name </param>
			/// <param name="hint"> Value hint </param>
			/// <param name="attributes"> Additional attribute </param>
			InstanceSerializer(const std::string_view& name, const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {});
			
			/// <summary> Virtual destructor </summary>
			virtual ~InstanceSerializer();

			/// <summary>
			/// Gives access to sub-serializers/fields
			/// </summary>
			/// <param name="recordElement"> Each sub-serializer will be reported by invoking this callback with serializer & corresonding target as parameters </param>
			/// <param name="targetAddr"> Serializer target object </param>
			virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, SerializableInstance* target)const override;

			/// <summary> Single instance, when you don't need the name </summary>
			static const InstanceSerializer* Instance();
		};
	};


	// Type details
	template<> inline void TypeIdDetails::GetParentTypesOf<ConfigurableResource>(const Callback<TypeId>& report) {
		report(TypeId::Of<Resource>());
		report(TypeId::Of<Serialization::Serializable>());
	}
}
