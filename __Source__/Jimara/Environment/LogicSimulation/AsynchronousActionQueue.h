#pragma once
#include "../Scene/Logic/LogicContext.h"


namespace Jimara {
	/// <summary>
	/// Single instance of a multithreaded asynchronous action queue for various tasks like 
	/// asynch resource loading and non-frame-critical general calculations.
	/// </summary>
	class JIMARA_API AsynchronousActionQueue : public virtual Object, public virtual ActionQueue<> {
	public:
		/// <summary>
		/// Retrieves a singleton instance of an AsynchronousActionQueue for given context
		/// <para/> Notes:
		/// <para/>		0. Context can not be nullptr!
		/// <para/>		1. Once created, AsynchronousActionQueue is automatically stored within the context's data object collection
		///				and there's no need to keep reference for one-off requests.
		/// </summary>
		/// <param name="context"> Scene context </param>
		/// <returns> Shared action queue </returns>
		static Reference<AsynchronousActionQueue> GetFor(SceneContext* context);

		/// <summary> Virtual destructor </summary>
		virtual ~AsynchronousActionQueue();

	private:
		// Constructor is private; actual concrete implementation is hidden
		AsynchronousActionQueue();
	};
}
