#pragma once
#include "ViewportLightSet.h"


namespace Jimara {
	/// <summary>
	/// Spatial hash grid of lights for stuff like forward-plus rendering and alike.
	/// <para/> Notes:
	/// <para/>		0. Read SceneLightGrid.glh for shader usage details;
	/// <para/>		1. It is crutial to wait for the UpdateJob() to finish each frame before 
	///				the bindings returned by BindingDescriptor are valid and safe to use.
	/// </summary>
	class JIMARA_API SceneLightGrid : public virtual Object {
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

		/// <summary> Job, that has to finish it's execution during the update cycle for the bindings to be up to date and safe to use </summary>
		JobSystem::Job* UpdateJob()const;

	private:
		// Actual implementation resides in here...
		struct Helpers;

		// Only the internals can access the constructor
		SceneLightGrid();
	};
}
