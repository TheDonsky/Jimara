#pragma once
#include "LightDescriptor.h"

namespace Jimara {
	/// <summary>
	/// Collection of all LightDescriptor::ViewportData objects for a specific ViewportDescriptor
	/// </summary>
	class JIMARA_API ViewportLightSet : public virtual Object {
	public:
		/// <summary>
		/// Gets shared instance of ViewportLightSet for given viewport descriptor
		/// </summary>
		/// <param name="viewport"> viewport descriptor </param>
		/// <returns> Cached ViewportLightSet </returns>
		static Reference<const ViewportLightSet> For(const ViewportDescriptor* viewport);

		/// <summary>
		/// Gets shared instance of ViewportLightSet for null viewport descriptor inside given scene context
		/// </summary>
		/// <param name="context"> Scene context </param>
		/// <returns> Cached ViewportLightSet </returns>
		static Reference<const ViewportLightSet> For(SceneContext* context);

		/// <summary>
		/// Thread-safe reader of light data
		/// </summary>
		class JIMARA_API Reader final {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="lightSet"> Light set to read content from </param>
			Reader(const ViewportLightSet* lightSet);

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="lightSet"> Light set to read content from </param>
			inline Reader(const Reference<ViewportLightSet>& lightSet) : Reader(lightSet.operator->()) {}

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="lightSet"> Light set to read content from </param>
			inline Reader(const Reference<const ViewportLightSet>& lightSet) : Reader(lightSet.operator->()) {}

			/// <summary> Destructor </summary>
			~Reader();

			/// <summary> Number of light descriptors </summary>
			size_t LightCount()const;

			/// <summary>
			/// Light descriptor by index
			/// </summary>
			/// <param name="index"> Index of the light descriptor [0 - LightCount()) </param>
			/// <returns> Light descriptor </returns>
			LightDescriptor* LightDescriptor(size_t index)const;

			/// <summary>
			/// View-specific data of the light descriptor with the same index;
			/// <para/> Notes:
			///		<para/> 0. Same as LightDescriptor(index)->GetViewportData(viewport), but this one is stored persistantly;
			///		<para/> 1. If GetViewportData() call returns nullptr, the value will be nullptr here too, so deal with it!
			/// </summary>
			/// <param name="index"> Index of the light descriptor [0 - LightCount()) </param>
			/// <returns> Viewport-specific light data </returns>
			const LightDescriptor::ViewportData* LightData(size_t index)const;

		private:
			// Reader lock
			std::shared_lock<std::shared_mutex> m_lock;

			// Data container
			const Reference<const Object> m_data;

			// Pointer to underlying data
			const void* m_descriptors;

			// Number of lights inside m_descriptors
			size_t m_lightCount;

			// Constructor with m_data as parameter...
			Reader(const Reference<Object> dataObject);
		};

	private:
		// Private stuff is stored here
		struct Helpers;

		// Constructor is private
		inline ViewportLightSet() {}
	};
}
