#pragma once
#include "../Core/TypeRegistration/ObjectFactory.h"
#include "../Graphics/GraphicsDevice.h"
#include "../Graphics/Data/ShaderBinaries/ShaderLoader.h"
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

			/// <summary> Shader module loader </summary>
			Graphics::ShaderLoader* shaderLoader = nullptr;

			/// <summary> Physics API instance </summary>
			Physics::PhysicsInstance* physicsInstance = nullptr;

			/// <summary> Audio device </summary>
			Audio::AudioDevice* audioDevice = nullptr;
		};

		/// <summary> Factory for creating configurable resources </summary>
		using Factory = ObjectFactory<ConfigurableResource, CreateArgs>;
	};


	// Type details
	template<> inline void TypeIdDetails::GetParentTypesOf<ConfigurableResource>(const Callback<TypeId>& report) {
		report(TypeId::Of<Resource>());
		report(TypeId::Of<Serialization::Serializable>());
	}
}
