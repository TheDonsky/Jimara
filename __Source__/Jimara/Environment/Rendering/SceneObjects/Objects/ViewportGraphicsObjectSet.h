#pragma once
#include "GraphicsObjectDescriptor.h"


namespace Jimara {
	/// <summary>
	/// Collection of all GraphicsObjectDescriptor::ViewportData objects for a specific ViewportDescriptor
	/// </summary>
	class JIMARA_API ViewportGraphicsObjectSet : public virtual Object {
	public:
		/// <summary>
		/// Gets shared instance of ViewportGraphicsObjectSet for given viewport descriptor
		/// </summary>
		/// <param name="viewport"> viewport descriptor </param>
		/// <returns> Cached ViewportGraphicsObjectSet </returns>
		static Reference<const ViewportGraphicsObjectSet> For(const ViewportDescriptor* viewport);

		/// <summary>
		/// Gets shared instance of ViewportGraphicsObjectSet for null viewport descriptor inside given scene context
		/// </summary>
		/// <param name="context"> Scene context </param>
		/// <returns> Cached ViewportGraphicsObjectSet </returns>
		static Reference<const ViewportGraphicsObjectSet> For(SceneContext* context);

		/// <summary>
		/// Information about an entry in Object set
		/// </summary>
		struct JIMARA_API ObjectInfo {
			/// <summary> Graphics object descriptor </summary>
			GraphicsObjectDescriptor* objectDescriptor = nullptr;

			/// <summary> Per-Viewport data (can be nullptr if the objectDescriptor does not return anything) </summary>
			const GraphicsObjectDescriptor::ViewportData* viewportData = nullptr;
		};

		/// <summary>
		/// Invoked each time graphics objects get added to the collection.
		/// <para/> Arguments passed to this callback will be an array of ObjectInfo-s and their count
		/// </summary>
		Event<const ObjectInfo*, size_t>& OnAdded()const;

		/// <summary>
		/// Invoked each time graphics objects get removed from the collection.
		/// <para/> Arguments passed to this callback will be an array of ObjectInfo-s and their count
		/// </summary>
		Event<const ObjectInfo*, size_t>& OnRemoved()const;

		/// <summary>
		/// Retrieves all entries within the graphics object collection
		/// </summary>
		/// <param name="inspectEntries"> Array of all ObjectInfo-s and their count will be passed as arguments to this callback </param>
		void GetAll(const Callback<const ObjectInfo*, size_t>& inspectEntries)const;

	private:
		// Private stuff is stored here
		struct Helpers;

		// Constructor is private
		inline ViewportGraphicsObjectSet() {}
	};
}
