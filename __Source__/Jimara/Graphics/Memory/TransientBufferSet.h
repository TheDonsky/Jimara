#pragma once
#include "../GraphicsDevice.h"


namespace Jimara {
	namespace Graphics {
		/// <summary>
		/// Sometime we need to use a buffer or two just as some intermediate memory for generating other, more permanent results.
		/// This is an utility that can come in handy for that case.
		/// <para/> Notes:
		///	<para/>		0. ArrayBuffer instances returned from TransientBufferSet will always have elem size set to 1; 
		///				that has mostly a "CPU-only" relevance and since these buffers are only intended to be used as 'shared scratch buffers',
		///				that aspect should not be relevant.
		/// <para/>		1. The buffers obtained from TransientBufferSet are not exclusive outside the LockBuffer scope 
		///				(read through LockBuffer() api for further details).
		/// </summary>
		class JIMARA_API TransientBufferSet : public virtual Object {
		public:
			/// <summary>
			/// Retrieves transient buffer set
			/// </summary>
			/// <param name="device"> Graphics device </param>
			/// <returns> Shared TransientBufferSet instance for given device </returns>
			static Reference<TransientBufferSet> Get(GraphicsDevice* device);

			/// <summary> Virtual destructor </summary>
			inline virtual ~TransientBufferSet() {}

			/// <summary>
			/// LockBuffer calls can recurse, but recursion depth can not exceed this value;
			/// <para/> Keep in mind, that each recursion level has it's own independent storage and unnecessary recursion will result in excessive VRAM usage!
			/// <para/> Recursion depth has the same total limit, even if one is operating on more than one TransientBufferSet objects at a time.
			/// </summary>
			static const constexpr size_t MAX_RECURSION_DEPTH = 256u;

			/// <summary>
			/// "Locks" a transient buffer and provides scope for a thread-wise exclusive use
			/// <para/> In plain text, no matter what, buffer passed to action will not be repeated if there are recursive LockBuffer() calls within the action's scope.
			/// <para/> There are no exclusivity guarantees, when it comes to invokations from different threads.
			/// </summary>
			/// <param name="minSize"> Minimal buffer size </param>
			/// <param name="action"> Receives the 'locked' buffer value as a parameter and for the duration of the scope </param>
			/// <returns> True, if the buffer gets obtained successfully and action is invoked </returns>
			bool LockBuffer(size_t minSize, const Callback<ArrayBuffer*>& action);

			/// <summary>
			/// "Locks" a transient buffer and provides scope for a thread-wise exclusive use
			/// <para/> In plain text, no matter what, buffer passed to action will not be repeated if there are recursive LockBuffer() calls within the action's scope.
			/// <para/> There are no exclusivity guarantees, when it comes to invokations from different threads.
			/// </summary>
			/// <typeparam name="ActionType"> Any callable, that receives a ArrayBuffer pointer as argument </typeparam>
			/// <param name="minSize"> Minimal buffer size </param>
			/// <param name="action"> Receives the 'locked' buffer value as a parameter and for the duration of the scope </param>
			/// <returns> True, if the buffer gets obtained successfully and action is invoked </returns>
			template<typename ActionType>
			inline bool LockBuffer(size_t minSize, const ActionType& action) {
				return LockBuffer(minSize, Callback<ArrayBuffer*>::FromCall(&action));
			}

			/// <summary> Current recursion depth, when it comes to nested LockBuffer calls </summary>
			static size_t RecursionDepth();

			/// <summary>
			/// Gets buffer by explicit scope index.
			/// <para/> By default, LockBuffer() sets buffer index to RecursionDepth(), gets Buffer by that index, increments it, invokes the action and decrements RecursionDepth().
			/// </summary>
			/// <param name="minSize"> Minimal buffer size </param>
			/// <param name="index"> Scope recursion depth index </param>
			/// <returns> Transient buffer by index (nullptr, if and only if some allocation fails) </returns>
			Reference<ArrayBuffer> GetBuffer(size_t minSize, size_t index = RecursionDepth());

		private:
			// TransientBufferSet Can not be externally allocated!
			inline TransientBufferSet() {}

			// Actual implementation goes here.
			struct Helpers;
		};
	}
}
