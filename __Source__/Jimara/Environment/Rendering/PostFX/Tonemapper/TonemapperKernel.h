#pragma once
#include "../../RenderStack.h"
#include "../../SimpleComputeKernel.h"


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
			REINHARD = 0u,

			/// <summary> Reinhard per luminocity (color / ((color.r + color.g + color.b) / 3 + 1)) </summary>
			REINHARD_LUMA = 1u,

			/// <summary> Not an actual type; just type count </summary>
			TYPE_COUNT = 2u
		};

		/// <summary> Enum attribute for Type options </summary>
		static const Object* TypeEnumAttribute();

		/// <summary>
		/// Creates tonemapper
		/// </summary>
		/// <param name="type"> Tonemapper algorithm </param>
		/// <param name="device"> Graphics device </param>
		/// <param name="shaderLoader"> Shader bytecode loader </param>
		/// <param name="maxInFlightCommandBuffers"> Maximal number of in-flight command buffers </param>
		/// <returns> New instance of a TonemapperKernel </returns>
		static Reference<TonemapperKernel> Create(
			Type type,
			Graphics::GraphicsDevice* device,
			Graphics::ShaderLoader* shaderLoader,
			size_t maxInFlightCommandBuffers);

		/// <summary> Virtual destructor </summary>
		virtual ~TonemapperKernel();

		/// <summary> Tonemapper type </summary>
		Type Algorithm()const;

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

		// Compute kernel wrapper
		const Reference<SimpleComputeKernel> m_kernel;

		// Target texture binding
		const Reference<Graphics::ResourceBinding<Graphics::TextureView>> m_target;

		// Constructor is... private. For reasons!
		TonemapperKernel(Type type, SimpleComputeKernel* kernel, Graphics::ResourceBinding<Graphics::TextureView>* target);
	};
}
