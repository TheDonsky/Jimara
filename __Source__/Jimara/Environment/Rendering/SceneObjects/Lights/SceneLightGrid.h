#pragma once
#include "ViewportLightSet.h"


namespace Jimara {
	class JIMARA_API SceneLightGrid : public virtual JobSystem::Job {
	public:
		/// <summary> Virtual destructor </summary>
		virtual ~SceneLightGrid();

		/// <summary>
		/// Shared instance per graphics context
		/// </summary>
		/// <param name="context"> "Owner" context </param>
		/// <returns> Instance, tied to the context </returns>
		static Reference<SceneLightGrid> GetFor(SceneContext* context);

		/// <summary>
		/// Shared instance per viewport
		/// </summary>
		/// <param name="viewport"> Scene viewport </param>
		/// <returns> Instance, tied to the context </returns>
		static Reference<SceneLightGrid> GetFor(const ViewportDescriptor* viewport);

		/// <summary> 
		/// Search functions, that report SceneLightGrid bindings by name
		/// <para/> Returned functions will stay valid till the moment SceneLightGrid goes out of scope
		/// </summary>
		Graphics::BindingSet::BindingSearchFunctions BindingDescriptor()const;

	private:
		// Actual implementation resides in here...
		struct Helpers;

		// Only the internals can access the constructor
		SceneLightGrid();
	};
}
