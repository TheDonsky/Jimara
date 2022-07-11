#pragma once
#include "../Math/BitMask.h"
#include "../Core/Object.h"
#include <shared_mutex>
#include <string>


namespace Jimara {
	/// <summary> Type definition for a generic layer (can be used for Graphics/Physics) </summary>
	typedef uint8_t Layer;

	/// <summary> Bitmask of layers </summary>
	typedef BitMask<Layer> LayerMask;

	/// <summary>
	/// While working on the game, one may want to have the Layers named and those names should be editable; This is the place to store them
	/// </summary>
	class JIMARA_API Layers : public virtual Object {
	public:
		/// <summary> Number of available layers (basically, same as the numer of different Layer values)
		inline static constexpr size_t Count() { return (sizeof(static_cast<Layers*>(nullptr)->m_layers) / sizeof(std::string)); }

		/// <summary> Main instance of Layers (you can create your own, but this one is one singleton instance some systems will hook into) </summary>
		static Layers* Main();

		/// <summary>
		/// To make Layers thread-safe, we have Reader and Writer classes;
		/// This one makes it possible to read current names
		/// </summary>
		class JIMARA_API Reader {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="target"> Layers object to read </param>
			inline Reader(const Layers* target = nullptr)
				: m_target(target != nullptr ? target : Main())
				, m_lock((target != nullptr ? target : Main())->m_lock) {}

			/// <summary> Destructor (useful for debugging and nothing else really) </summary>
			inline ~Reader() {}

			/// <summary>
			/// Layer name
			/// </summary>
			/// <param name="layer"> Layer, name of which we're interested in </param>
			/// <returns> Name of the layer, according to the target object </returns>
			const std::string& operator[](Layer layer)const { return m_target->m_layers[static_cast<size_t>(layer)]; }

		private:
			// Target and read lock
			const Reference<const Layers> m_target;
			const std::shared_lock<std::shared_mutex> m_lock;
		};

		/// <summary>
		/// To make Layers thread-safe, we have Reader and Writer classes;
		/// This one makes it possible to alter layer names
		/// </summary>
		class JIMARA_API Writer {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="target"> Layers object to edit </param>
			inline Writer(Layers* target = nullptr)
				: m_target(target != nullptr ? target : Main())
				, m_lock((target != nullptr ? target : Main())->m_lock) {}

			/// <summary> Destructor (useful for debugging and nothing else really) </summary>
			inline ~Writer() {}

			/// <summary>
			/// Layer name reference
			/// </summary>
			/// <param name="layer"> Layer, name of which we're interested in </param>
			/// <returns> Name of the layer, according to the target object </returns>
			std::string& operator[](Layer layer)const { return m_target->m_layers[static_cast<size_t>(layer)]; }

		private:
			// Target and write lock
			const Reference<Layers> m_target;
			const std::unique_lock<std::shared_mutex> m_lock;
		};

		/// <summary>
		/// When serializing a Layer field, provide this attribute to the serializer to display options correctly
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
		/// When serializing a LayerMask field, provide this attribute to the serializer to display it as a bitmask
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
		std::string m_layers[1 << (sizeof(Layer) * 8)];

		// Reader-Writer lock
		mutable std::shared_mutex m_lock;
	};
}
