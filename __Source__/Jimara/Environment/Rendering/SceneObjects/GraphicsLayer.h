#pragma once
#include "../../Scene/Scene.h"


namespace Jimara {
	/// <summary> Layer of a graphics object (various renderers may choose to include or exclude layers) </summary>
	typedef uint8_t GraphicsLayer;

	/// <summary> Bitmask of graphics layers </summary>
	typedef BitMask<GraphicsLayer> GraphicsLayerMask;

	/// <summary>
	/// While working on the game, one may want to have the GraphicsLayers named and those names should be editable; This is the place to store them
	/// </summary>
	class JIMARA_API GraphicsLayers : public virtual Object {
	public:
		/// <summary> Number of available graphics layers (basically, same as the numer of different GraphicsLayer values)
		inline static constexpr size_t Count() { return (sizeof(static_cast<GraphicsLayers*>(nullptr)->m_layers) / sizeof(std::string)); }

		/// <summary> Main instance of GraphicsLayers (you can create your own, but this one is one singleton instance some systems will hook into) </summary>
		static GraphicsLayers* Main();

		/// <summary>
		/// To make GraphicsLayers thread-safe, we have Reader and Writer classes;
		/// This one makes it possible to read current names
		/// </summary>
		class Reader {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="target"> GraphicsLayers object to read </param>
			inline Reader(const GraphicsLayers* target = nullptr)
				: m_target(target != nullptr ? target : Main())
				, m_lock((target != nullptr ? target : Main())->m_lock) {}

			/// <summary> Destructor (useful for debugging and nothing else really) </summary>
			inline ~Reader() {}

			/// <summary>
			/// Graphics layer name
			/// </summary>
			/// <param name="layer"> Layer, name of which we're interested in </param>
			/// <returns> Name of the layer, according to the target object </returns>
			const std::string& operator[](GraphicsLayer layer)const { return m_target->m_layers[static_cast<size_t>(layer)]; }

		private:
			// Target and read lock
			const Reference<const GraphicsLayers> m_target;
			const std::shared_lock<std::shared_mutex> m_lock;
		};

		/// <summary>
		/// To make GraphicsLayers thread-safe, we have Reader and Writer classes;
		/// This one makes it possible to alter layer names
		/// </summary>
		class Writer {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="target"> GraphicsLayers object to edit </param>
			inline Writer(GraphicsLayers* target = nullptr) 
				: m_target(target != nullptr ? target : Main())
				, m_lock((target != nullptr ? target : Main())->m_lock) {}

			/// <summary> Destructor (useful for debugging and nothing else really) </summary>
			inline ~Writer() {}

			/// <summary>
			/// Graphics layer name reference
			/// </summary>
			/// <param name="layer"> Layer, name of which we're interested in </param>
			/// <returns> Name of the layer, according to the target object </returns>
			std::string& operator[](GraphicsLayer layer)const { return m_target->m_layers[static_cast<size_t>(layer)]; }

		private:
			// Target and write lock
			const Reference<GraphicsLayers> m_target;
			const std::unique_lock<std::shared_mutex> m_lock;
		};

		/// <summary>
		/// When serializing a GraphicsLayer field, provide this attribute to the serializer to display options correctly
		/// </summary>
		class LayerAttribute : public virtual Object {
		public:
			/// <summary> Singleton instance of the attribute (you can create your own, but this may come in handy) </summary>
			inline static const LayerAttribute* Instance() {
				static const LayerAttribute instance;
				return &instance;
			}
		};

		/// <summary>
		/// When serializing a GraphicsLayerMask field, provide this attribute to the serializer to display it as a bitmask
		/// </summary>
		class LayerMaskAttribute : public virtual Object {
		public:
			/// <summary> Singleton instance of the attribute (you can create your own, but this may come in handy) </summary>
			inline static const LayerMaskAttribute* Instance() {
				static const LayerMaskAttribute instance;
				return &instance;
			}
		};

	private:
		// Layer names
		std::string m_layers[1 << (sizeof(GraphicsLayer) * 8)];

		// Reader-Writer lock
		mutable std::shared_mutex m_lock;
	};
}
