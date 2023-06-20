#include "../RenderStack.h"
#include "../ViewportDescriptor.h"


namespace Jimara {
	/// <summary>
	/// Renderer that renders skybox based on an HDRI texture
	/// </summary>
	class JIMARA_API HDRISkyboxRenderer : public virtual RenderStack::Renderer {
	public:
		/// <summary>
		/// Creates HDRI-based skybox renderer
		/// </summary>
		/// <param name="viewport"> Viewport descriptor </param>
		/// <returns> New instance of an HDRISkyboxRenderer </returns>
		static Reference<HDRISkyboxRenderer> Create(const ViewportDescriptor* viewport);

		/// <summary> Virtual destructor </summary>
		virtual ~HDRISkyboxRenderer();

		/// <summary>
		/// Sets environment map
		/// </summary>
		/// <param name="hdriSampler"> HDRI texture sampler </param>
		void SetEnvironmentMap(Graphics::TextureSampler* hdriSampler);

		/// <summary>
		/// Sets environment color multiplier
		/// </summary>
		/// <param name="color"> Color from image will be multiplied byu this value </param>
		void SetColorMultiplier(const Vector4& color);

	private:
		// Constructor needs to be private...
		inline HDRISkyboxRenderer() { }

		// Actual implementation hides behind this struct...
		struct Helpers;
	};
}
