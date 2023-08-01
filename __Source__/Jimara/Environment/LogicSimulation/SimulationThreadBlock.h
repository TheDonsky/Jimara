#pragma once
#include "../Scene/Logic/LogicContext.h"


namespace Jimara {
	/// <summary>
	/// A simple thread block that can be accessed and shared per-context
	/// </summary>
	class JIMARA_API SimulationThreadBlock : public virtual Object, public virtual ThreadBlock {
	public:
		/// <summary> Virtual destructor </summary>
		inline virtual ~SimulationThreadBlock() {}

		/// <summary> Max thread count that is 'recommended' </summary>
		inline size_t DefaultThreadCount()const { return m_defaultThreadCount; }

		/// <summary>
		/// Gets shared instance of a SynchronousThreadBlock
		/// </summary>
		/// <param name="context"> Scene context (can not be nullptr) </param>
		/// <returns> Shared instance </returns>
		static Reference<SimulationThreadBlock> GetFor(SceneContext* context);

	private:
		// Max thread count that is 'recommended'
		const size_t m_defaultThreadCount = Math::Max(std::thread::hardware_concurrency() / 2u, 1u);

		// Constructor can only be called by GetFor() implementation
		inline SimulationThreadBlock() {}
	};
}
