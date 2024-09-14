#pragma once
#include "../../RenderStack.h"
#include "../../SimpleComputeKernel.h"
#include "../../../../Math/GraphicsCurves.h"


namespace Jimara {
	/// <summary>
	/// Tonemapper PostFX
	/// </summary>
	class JIMARA_API TonemapperKernel : public virtual Object {
	public:
		/// <summary>
		/// Tonemapper algorithm
		/// </summary>
		enum class JIMARA_API Type : uint8_t {

			/// <summary> Reinhard per channel (color / (color + 1)) </summary>
			REINHARD_PER_CHANNEL = 0u,

			/// <summary> Reinhard per luminocity (color / ((color.r + color.g + color.b) / 3 + 1)) </summary>
			REINHARD_LUMINOCITY = 1u,

			/// <summary> ACES (Filmic 'S' curve approximation) </summary>
			ACES = 2u,

			/// <summary> ACES (Filmic 'S' curve approximation) </summary>
			ACES_APPROX = 3u,

			/// <summary> Custom filmic curve </summary>
			CUSTOM_CURVE = 4u,

			/// <summary> Not an actual type; just type count </summary>
			TYPE_COUNT = 5u,


			/// <summary> No tonemapping </summary>
			NONE = TYPE_COUNT
		};

		/// <summary> Parent class for all per-type settings </summary>
		class JIMARA_API Settings;

		/// <summary> Settings for REINHARD_PER_CHANNEL & REINHARD_LUMINOCITY </summary>
		class JIMARA_API ReinhardSettings;

		/// <summary> Settings for ACES_APPROX </summary>
		class JIMARA_API ACESSettings;

		/// <summary> Settings for CUSTOM_CURVE </summary>
		class JIMARA_API CustomCurveSettings;

		/// <summary> Enum attribute for Type options </summary>
		static const Object* TypeEnumAttribute();

		/// <summary>
		/// Creates tonemapper
		/// </summary>
		/// <param name="type"> Tonemapper algorithm </param>
		/// <param name="device"> Graphics device </param>
		/// <param name="shaderLibrary"> Shader bytecode loader </param>
		/// <param name="maxInFlightCommandBuffers"> Maximal number of in-flight command buffers </param>
		/// <returns> New instance of a TonemapperKernel </returns>
		static Reference<TonemapperKernel> Create(
			Type type,
			Graphics::GraphicsDevice* device,
			ShaderLibrary* shaderLibrary,
			size_t maxInFlightCommandBuffers);

		/// <summary> Virtual destructor </summary>
		virtual ~TonemapperKernel();

		/// <summary> Tonemapper type </summary>
		Type Algorithm()const;

		/// <summary> Kernel parameters </summary>
		Settings* Configuration()const;

		/// <summary> Current target texture </summary>
		Graphics::TextureView* Target()const;

		/// <summary>
		/// Sets target texture
		/// </summary>
		/// <param name="target"> Target texture view </param>
		void SetTarget(Graphics::TextureView* target);

		/// <summary>
		/// Dispatches the underlying pipeline(s)
		/// </summary>
		/// <param name="commandBuffer"> In-flight command buffer </param>
		void Execute(const Graphics::InFlightBufferInfo& commandBuffer);


	private:
		// Type
		const Type m_type;

		// Settings buffer
		const Reference<Settings> m_settings;

		// Compute kernel wrapper
		const Reference<SimpleComputeKernel> m_kernel;

		// Target texture binding
		const Reference<Graphics::ResourceBinding<Graphics::TextureView>> m_target;

		// Constructor is... private. For reasons!
		TonemapperKernel(Type type, Settings* settings, SimpleComputeKernel* kernel, Graphics::ResourceBinding<Graphics::TextureView>* target);

		// Private stuff is here...
		struct Helpers;
	};


	/// <summary>
	/// Parent class for all per-type settings
	/// </summary>
	class JIMARA_API TonemapperKernel::Settings : public virtual Object, public virtual Serialization::Serializable {
	public:
		/// <summary>
		/// Changes after 'GetFields' calls are not automatically synched on GPU; 
		/// use this call to update relevant buffers on GPU .
		/// </summary>
		virtual void Apply() = 0;
	};


	/// <summary>
	/// Settings for ACES_APPROX
	/// </summary>
	class JIMARA_API TonemapperKernel::ReinhardSettings final : public virtual Settings {
	public:
		/// <summary> Radiance value that will be mapped to 1 </summary>
		float maxWhite = 1.75f;

		/// <summary> 'Tint' of the max white value; generally, white is recommended, but anyone is free to experiment </summary>
		Vector3 maxWhiteTint = Vector3(1.0f);

		/// <summary>
		/// Gives access to fields
		/// </summary>
		/// <param name="recordElement"> Fields will be reported by invoking this callback with serializer & corresonding target as parameters </param>
		virtual void GetFields(Callback<Serialization::SerializedObject> recordElement) final override;

		/// <summary>
		/// Changes after 'GetFields' calls are not automatically synched on GPU; 
		/// use this call to update relevant buffers on GPU .
		/// </summary>
		virtual void Apply() final override;

	private:
		// Settings buffer
		const Graphics::BufferReference<Vector3> m_settingsBuffer;

		// Constructor is private and only accessible to TonemapperKernel
		inline ReinhardSettings(Graphics::Buffer* buffer) : m_settingsBuffer(buffer) { 
			assert(m_settingsBuffer != nullptr); 
		}
		friend class TonemapperKernel;
	};


	/// <summary>
	/// Settings for REINHARD_PER_CHANNEL & REINHARD_LUMINOCITY
	/// </summary>
	class JIMARA_API TonemapperKernel::ACESSettings final : public virtual Settings {
	public:
		/// <summary>
		/// Gives access to fields
		/// </summary>
		/// <param name="recordElement"> Fields will be reported by invoking this callback with serializer & corresonding target as parameters </param>
		inline virtual void GetFields(Callback<Serialization::SerializedObject> recordElement) final override {}

		/// <summary>
		/// Changes after 'GetFields' calls are not automatically synched on GPU; 
		/// use this call to update relevant buffers on GPU .
		/// </summary>
		inline virtual void Apply() final override {}

	private:
		// Constructor is private and only accessible to TonemapperKernel
		inline ACESSettings() {}
		friend class TonemapperKernel;
	};

	/// <summary>
	/// Settings for CUSTOM_CURVE
	/// </summary>
	class JIMARA_API TonemapperKernel::CustomCurveSettings final : public virtual Settings {
	public:
		/// <summary> Radiance value that will be mapped to 1 </summary>
		float maxWhite = 1.75f;

		/// <summary> 'Tint' of the max white value; generally, white is recommended, but anyone is free to experiment </summary>
		Vector3 maxWhiteTint = Vector3(1.0f);

		/// <summary> Color response curve </summary>
		GraphicsTimelineCurve<Vector3> responseCurve;

		/// <summary>
		/// Gives access to fields
		/// </summary>
		/// <param name="recordElement"> Fields will be reported by invoking this callback with serializer & corresonding target as parameters </param>
		virtual void GetFields(Callback<Serialization::SerializedObject> recordElement) final override;

		/// <summary>
		/// Changes after 'GetFields' calls are not automatically synched on GPU; 
		/// use this call to update relevant buffers on GPU .
		/// </summary>
		virtual void Apply() final override;

	private:
		// Settings buffer
		const Graphics::BufferReference<Vector3> m_settingsBuffer;

		// Response curve binding
		const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> m_responseCurveBinding =
			Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();

		// Constructor is private and only accessible to TonemapperKernel
		inline CustomCurveSettings(Graphics::GraphicsDevice* device, Graphics::Buffer* buffer);
		friend class TonemapperKernel;
	};
}
