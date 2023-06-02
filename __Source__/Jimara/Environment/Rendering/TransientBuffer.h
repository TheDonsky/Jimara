#pragma once
#include "../../Graphics/GraphicsDevice.h"


namespace Jimara {
	/// <summary>
	/// Sometime we need to use a texture just as an intermediate buffer for generating other, more permanent results.
	/// This is an utility that can come in handy for that case.
	/// <para/> Notes:
	///	<para/>		0. ArrayBuffer instances returned by TransientBuffer will always have elem size set to 1; 
	///				that has mostly a "CPU-only" relevance and since these buffers are only intended to be used as 'shared scratch buffers',
	///				that aspect should not be relevant.
	/// <para/>		1. Similatily to the transient images, the buffers returned by TransientBuffer are not exclusive and
	///				only way to reliably have different buffers is to use different indices. Otherwise, same cached instance is returned to all users.
	/// </summary>
	class TransientBuffer : public virtual Object {
	public:
		/// <summary>
		/// Gets shared TransientBuffer instance
		/// </summary>
		/// <param name="device"> Graphics device </param>
		/// <param name="index"> Unique buffer index for the device </param>
		/// <returns> Shared transient buffer </returns>
		static Reference<TransientBuffer> Get(Graphics::GraphicsDevice* device, size_t index);

		/// <summary>
		/// Gets or reallocates buffer with some minimum size
		/// </summary>
		/// <param name="minSize"> Minimal size for the buffer </param>
		/// <returns> Shared buffer </returns>
		Reference<Graphics::ArrayBuffer> GetBuffer(size_t minSize);

	private:
		// Device
		const Reference<Graphics::GraphicsDevice> m_device;

		// Lock for internal buffer reference
		SpinLock m_lock;

		// Underlying shared buffer
		Reference<Graphics::ArrayBuffer> m_buffer;

		// Constructor
		inline TransientBuffer(Graphics::GraphicsDevice* device) : m_device(device) {}
	};
}
