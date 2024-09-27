#pragma once
#include "../../Data/Serialization/ItemSerializers.h"
#include "../../Core/Helpers.h"
#include "../../OS/IO/Path.h"
#include "../GraphicsDevice.h"
#include <set>



namespace Jimara {
	namespace Graphics {
		/// <summary>
		/// "Shared" instance of a constant ConstantBuffer Binding that has a fixed content
		/// <para /> Note: If user modifies the contents, system will have no way to know and that would be bad. So do not do that crap!
		/// </summary>
		/// <param name="bufferData"> Buffer content </param>
		/// <param name="bufferSize"> Content size </param>
		/// <param name="device"> Graphics device </param>
		/// <returns> Shared ConstantBufferBinding instance </returns>
		JIMARA_API Reference<const ResourceBinding<Buffer>> SharedConstantBufferBinding(const void* bufferData, size_t bufferSize, GraphicsDevice* device);

		/// <summary>
		/// "Shared" instance of a constant ConstantBufferBinding that has a fixed content
		/// <para /> Note: If user modifies the contents, system will have no way to know and that would be bad. So do not do that crap!
		/// </summary>
		/// <param name="content"> Buffer content </param>
		/// <param name="device"> Graphics device </param>
		/// <returns> Shared ConstantBufferBinding instance </returns>
		template<typename BufferType>
		inline static Reference<const ResourceBinding<Buffer>> SharedConstantBufferBinding(const BufferType& content, GraphicsDevice* device) {
			return SharedConstantBufferBinding(&content, sizeof(BufferType), device);
		}

		/// <summary>
		/// "Shared" instance of a constant TextureSampler Binding that binds to a single pixel texture with given color
		/// <para /> Note: If user modifies the contents, system will have no way to know and that would be bad. So do not do that crap!
		/// </summary>
		/// <param name="color"> Texture color </param>
		/// <param name="device"> Graphics device </param>
		/// <returns> Shared TextureSamplerBinding instance </returns>
		JIMARA_API Reference<const ResourceBinding<TextureSampler>> SharedTextureSamplerBinding(const Vector4& color, GraphicsDevice* device);
	}
}
