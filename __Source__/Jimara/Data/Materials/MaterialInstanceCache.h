#pragma once
#include "Material.h"
#include "../ShaderLibrary.h"


namespace Jimara {
	/// <summary>
	/// Cache for shared material instances
	/// </summary>
	class JIMARA_API MaterialInstanceCache {
	public:
		/// <summary>
		/// Information about a field override
		/// </summary>
		/// <typeparam name="FieldType"> Type of a field that's being overriden </typeparam>
		template<typename FieldType>
		struct FieldOverride {
			/// <summary> Field name (JM_MaterialProperty name) </summary>
			std::string fieldName;

			/// <summary> Override value </summary>
			FieldType overrideValue = {};
		};

		/// <summary>
		/// List of overriden fields
		/// </summary>
		/// <typeparam name="FieldType"> Type of a field that's being overriden </typeparam>
		template<typename FieldType> 
		using OverrideList = Stacktor<FieldOverride<FieldType>, 4u>;

		/// <summary>
		/// Material parameter overrides
		/// </summary>
		struct JIMARA_API Overrides : public virtual Object {
			/// <summary> float value overrides </summary>
			OverrideList<float> fp32;

			/// <summary> double value overrides </summary>
			OverrideList<double> fp64;

			/// <summary> int32_t value overrides </summary>
			OverrideList<int32_t> int32;

			/// <summary> uint32_t value overrides </summary>
			OverrideList<uint32_t> uint32;

			/// <summary> int64_t value overrides </summary>
			OverrideList<int64_t> int64;

			/// <summary> uint64_t value overrides </summary>
			OverrideList<uint64_t> uint64;

			/// <summary> bool value overrides </summary>
			OverrideList<bool> bool32;

			/// <summary> Vector2 value overrides </summary>
			OverrideList<Vector2> vec2;

			/// <summary> Vector3 value overrides </summary>
			OverrideList<Vector3> vec3;

			/// <summary> Vector4 value overrides </summary>
			OverrideList<Vector4> vec4;

			/// <summary> Matrix4 value overrides </summary>
			OverrideList<Matrix4> mat4;

			/// <summary> Texture value overrides </summary>
			OverrideList<Reference<Graphics::TextureSampler>> textures;

			/// <summary> Virtual destructor </summary>
			inline virtual ~Overrides() {}
		};

		/// <summary>
		/// Shared instance of a material with overridable parameters
		/// <para/> Reference to optional overrides will be kept alive while at least one instance exists;
		/// <para/> If overrides are provided, the contents can not change till the moment the call returns 
		/// and changes for the same override pointer will be ignored within subsequent calls.
		/// </summary>
		/// <param name="device"> Graphics device </param>
		/// <param name="bindlessBuffers"> Bindless buffer set </param>
		/// <param name="bindlessSamplers"> Bindless sampler set </param>
		/// <param name="litShader"> Lit-Shader </param>
		/// <param name="overrides"> Overrides [optional] </param>
		/// <returns> Shared instance of this material for the configuration </returns>
		static Reference<const Material::Instance> SharedInstance(
			Graphics::GraphicsDevice* device,
			Graphics::BindlessSet<Graphics::ArrayBuffer>* bindlessBuffers,
			Graphics::BindlessSet<Graphics::TextureSampler>* bindlessSamplers,
			const Material::LitShader* litShader,
			const Overrides* overrides = nullptr);

	private:
		inline MaterialInstanceCache() {}
	};
}


