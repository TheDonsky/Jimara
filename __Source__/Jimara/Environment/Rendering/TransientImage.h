#pragma once
#include "../../Graphics/GraphicsDevice.h"

namespace Jimara {
	/// <summary>
	/// Sometime we need to use a texture just as an intermediate buffer for generating other, more permanent results.
	/// This is an utility that can come in handy for that case.
	/// <para/> Example Usage:
	///		<para/> Let's say, you want to render a variance shadow map. 
	///		One approach would be to create a depth buffer, perform a depth only pass and generate the shadowmap using VarianceShadowMapper.
	///		All this is fine and good, but we're holding a depth buffer that is no longer of any use to us and contents of which are no more relevant.
	///		For cases like that, once could retrieve TransientImage for that depth-only pass, calculate the variance map and rest assured that the same depth buffer
	///		will be used by every single shadowmapper that has the same resolution without additional allocations, which is nice.
	/// </summary>
	class JIMARA_API TransientImage : public virtual Object {
	public:
		/// <summary>
		/// Retrieves a cached "transient" testure, that can be used freely in a command buffer, 
		/// as long as one does not care if anyone modifies it afterwards.
		/// </summary>
		/// <param name="device"> Graphics device </param>
		/// <param name="type"> Texture type </param>
		/// <param name="format"> Texture format </param>
		/// <param name="size"> Texture size </param>
		/// <param name="arraySize"> Texture array slice count </param>
		/// <param name="sampleCount"> Desired multisampling (if the device does not support this amount, some lower number may be chosen) </param>
		/// <returns> Shared TransientImage for given parametrers </returns>
		static Reference<TransientImage> Get(Graphics::GraphicsDevice* device, 
			Graphics::Texture::TextureType type, Graphics::Texture::PixelFormat format,
			Size3 size, uint32_t arraySize, Graphics::Texture::Multisampling sampleCount);

		/// <summary> Virtual destructor </summary>
		inline virtual ~TransientImage() {}

		/// <summary> Shared texture </summary>
		inline Graphics::Texture* Texture()const { return m_texture; }

	private:
		// Shared texture
		const Reference<Graphics::Texture> m_texture;

		// Constructor is private
		inline TransientImage(Graphics::Texture* texture) : m_texture(texture) {}

		// Private stuff
		struct Helpers;
	};
}
