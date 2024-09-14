#pragma once
#include "../../../Scene/Scene.h"


namespace Jimara {
	/// <summary> CPU-side definition and a few helpers for gl Jimara_RNG_t </summary>
	class JIMARA_API GraphicsRNG : public virtual Object {
	public:
		/// <summary> CPU-side definition for gl Jimara_RNG_t </summary>
		struct JIMARA_API State {
			alignas(4) uint32_t a = 0u;         // Bytes [0 - 4)
			alignas(4) uint32_t b = 1u;         // Bytes [4 - 8)
			alignas(4) uint32_t c = 2u;         // Bytes [8 - 12)
			alignas(4) uint32_t d = 3u;         // Bytes [12 - 16)
			alignas(4) uint32_t e = 4u;         // Bytes [16 - 20)
			alignas(4) uint32_t counter = 0u;   // Bytes [20 - 24)
			alignas(4) uint32_t pad_0 = 0u;     // Bytes [24 - 28)
			alignas(4) uint32_t pad_1 = 0u;     // Bytes [28 - 32)
		};
		static_assert(sizeof(State) == 32u);

		/// <summary>
		/// Retrieves a shared instance of a GraphicsRNG object for given device and shader loader pair
		/// </summary>
		/// <param name="device"> Graphics device </param>
		/// <param name="shaderLibrary"> Shader library (used for generating initial seed states with a compute shader) </param>
		/// <returns> Shared instance of a GraphicsRNG </returns>
		static Reference<GraphicsRNG> GetShared(Graphics::GraphicsDevice* device, ShaderLibrary* shaderLibrary);

		/// <summary>
		/// Retrieves or creates a shared instance of GraphicsRNG for given logic context
		/// <para/> This one will stay alive till the context goes out of scope even if the user does not keep the reference.
		/// </summary>
		/// <param name="context"> Scene context </param>
		/// <returns> Shared instance of a GraphicsRNG </returns>
		static Reference<GraphicsRNG> GetShared(SceneContext* context);

		/// <summary> Virtual destructor </summary>
		virtual ~GraphicsRNG();

		/// <summary>
		/// Gets an initialized(seeded) buffer of at least given size.
		/// <para/> Note that if some other user requests a bigger buffer, the stored buffer may change. 
		///		To prevent keeping old buffers unnecessarily, it's adviced to re-request the buffer every once in a while. 
		///		(Saying that, the underlying buffer sizes grow as powers of 2, the RNG-s will be unrelated and ignoring that will never gonna cause any problems)
		/// </summary>
		/// <param name="minSize"> Minimal amount of RNG-s in the buffer </param>
		/// <returns> RNG state buffer </returns>
		Graphics::ArrayBufferReference<State> GetBuffer(size_t minSize);

		/// <summary> Type-cast to the underlying RNG state buffer (before the initial GetBuffer() call, the size will be 0) </summary>
		inline operator Graphics::ArrayBufferReference<State>()const {
			std::unique_lock<SpinLock> lock(m_bufferLock);
			const Graphics::ArrayBufferReference<State> ref = m_buffer;
			return ref;
		}

	private:
		// Lock for m_buffer
		mutable SpinLock m_bufferLock;

		// RNG state buffer
		Graphics::ArrayBufferReference<State> m_buffer;

		// Nobody but the GetShared() methods are allowed to create buffers
		GraphicsRNG();

		// Private stuff is hidden beneath this
		struct Helpers;
	};
}
