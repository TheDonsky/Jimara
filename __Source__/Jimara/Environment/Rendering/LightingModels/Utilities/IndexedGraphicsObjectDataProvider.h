#pragma once
#include "../GraphicsObjectPipelines.h"


namespace Jimara {
	/// <summary>
	/// Custom viewport data provider for GraphicsObjectPipelines, 
	/// that adds a distinct index value and corresponding constant buffer binding to the viewport data objects from graphics object descriptors.
	/// </summary>
	class JIMARA_API IndexedGraphicsObjectDataProvider : public virtual GraphicsObjectPipelines::CustomViewportDataProvider {
	public:
		/// <summary> GraphicsObjectDescriptor::ViewportData with a distinct index </summary>
		class JIMARA_API ViewportData;

		/// <summary> Descriptor and unique identifier for IndexedGraphicsObjectDataProvider creation and caching </summary>
		struct JIMARA_API Descriptor;

		/// <summary> Virtual destructor </summary>
		inline virtual ~IndexedGraphicsObjectDataProvider() {}

		/// <summary>
		/// Gives access to a shared instance of IndexedGraphicsObjectDataProvider based on the descriptor
		/// </summary>
		/// <param name="id"> Shared instane identifier </param>
		/// <returns> Shared IndexedGraphicsObjectDataProvider </returns>
		static Reference<IndexedGraphicsObjectDataProvider> GetFor(const Descriptor& id);

	private:
		// Constructor is private
		inline IndexedGraphicsObjectDataProvider() {}

		// Most of the implementation resides in here
		struct Helpers;
	};


	/// <summary>
	/// GraphicsObjectDescriptor::ViewportData with a distinct index
	/// </summary>
	class JIMARA_API IndexedGraphicsObjectDataProvider::ViewportData : public virtual GraphicsObjectDescriptor::ViewportData {
	public:
		/// <summary> Virtual destructor </summary>
		inline virtual ~ViewportData() {}

		/// <summary> 
		/// Base/Underlying viewport data from GraphicsObjectDescriptor;
		/// <para/> This viewport data will share all bindings and public properties with BaseData, 
		/// with the addition of the index and a cbuffer-binding for the index that can be found by Descriptor::customIndexBindingName name.
		/// </summary>
		inline const GraphicsObjectDescriptor::ViewportData* BaseData()const { return m_baseData; }

		/// <summary>
		/// Distinct index given to the viewport data.
		/// <para/> Indices will be different for the coexisting ViewportData objects created by the same IndexedGraphicsObjectDataProvider;
		/// Having said that, when ViewportData objects go out of scope, their indices are generally returned to the common pool 
		/// and will be subsequently reused when new GraphicsObjectDescriptors appear.
		/// <para/> Values of the indices will start from 0 and grow only when we have ViewportData objects using all allocated indices.
		/// This way, we guarantee, that the maximum index at any point is roughly constrained to be on the order of 
		/// maximal number of entries within corresponding GraphicsObjectDescriptor::Set.
		/// </summary>
		inline uint32_t Index()const { return m_index; }

	private:
		// Base data
		const Reference<const GraphicsObjectDescriptor::ViewportData> m_baseData;

		// Distinct index
		const uint32_t m_index;

		// Constructor is private
		inline ViewportData(const GraphicsObjectDescriptor::ViewportData* base, uint32_t index) : m_baseData(base), m_index(index) {}

		// Underlying implementation can access the constructor
		friend struct Helpers;
	};


	/// <summary>
	/// Descriptor and unique identifier for IndexedGraphicsObjectDataProvider creation and caching
	/// </summary>
	struct JIMARA_API IndexedGraphicsObjectDataProvider::Descriptor final {
		/// <summary> Graphics object set </summary>
		Reference<const GraphicsObjectDescriptor::Set> graphicsObjects;

		/// <summary> Renderer frustrum descriptor (not strictly necessary, but providing this will keep the maximum index relatively low) </summary>
		Reference<const RendererFrustrumDescriptor> frustrumDescriptor;

		/// <summary> Custom index for searching binding search of the ViewportData::Index constant buffer </summary>
		std::string customIndexBindingName;

		/// <summary>
		/// Compares two descriptors
		/// </summary>
		/// <param name="other"> Other descriptor </param>
		/// <returns> True, if the descriptors are the same </returns>
		bool operator==(const Descriptor& other)const;

		/// <summary>
		/// Compares two descriptors
		/// </summary>
		/// <param name="other"> Other descriptor </param>
		/// <returns> True, if the descriptors are the same </returns>
		bool operator!=(const Descriptor& other)const;
	};
}

namespace std {
	/// <summary>
	/// Jimara::IndexedGraphicsObjectDataProvider::Descriptor hasher
	/// </summary>
	template<>
	struct hash<Jimara::IndexedGraphicsObjectDataProvider::Descriptor> {
		/// <summary>
		/// Computes hash-function of Jimara::IndexedGraphicsObjectDataProvider::Descriptor
		/// </summary>
		/// <param name="desc"> Jimara::IndexedGraphicsObjectDataProvider::Descriptor </param>
		/// <returns> Hash of the descriptor </returns>
		size_t operator()(const Jimara::IndexedGraphicsObjectDataProvider::Descriptor& desc)const;
	};
}
