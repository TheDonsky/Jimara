#pragma once
namespace Jimara {
	namespace Graphics {
		class Buffer;
		class ArrayBuffer;
		class Texture;
		class TextureView;
		class TestureSampler;
		template<typename DataType> class BindlessSet;
	}
}
#include "../GraphicsDevice.h"


namespace Jimara {
	namespace Graphics {
		/// <summary>
		/// Set of bindless descriptors for, well... bindless access
		/// <para/> Notes: 
		///		<para/> 0. Initial implementation only permits using bindless descriptor sets that consume entire binding sets and are bound at slot 0
		///		(This is an artificial limitation, imposed by the engine to keep the initial code simpler and performance as high as possible);
		///		<para/> 1. For now, only supported data types are TextureSampler and ArrayBuffer;
		///		<para/> 2. Binding name aliazing inside shaders is permitted, but the descriptor should be visible from all stages;
		///		<para/> 3. Feel free to insert as many objects in the binding as you desire, but there actually is a hard limit on the maximal number of available indices.
		/// </summary>
		/// <typeparam name="DataType"> Bound resource type (TextureSampler, for example) </typeparam>
		template<typename DataType> 
		class JIMARA_API BindlessSet : public virtual Object {
		public:
			/// <summary> Index to object link inside a bindless descriptor set </summary>
			class JIMARA_API Binding : public virtual Object {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="index"> Index within the bindless array </param>
				inline Binding(uint32_t index) : m_index(index) {}

				/// <summary> Index within the bindless array inside the shaders </summary>
				inline uint32_t Index()const { return m_index; }

				/// <summary> Object, associated with the index </summary>
				virtual DataType* BoundObject()const = 0;

			private:
				// Index within the bindless array
				const uint32_t m_index;
			};

			/// <summary> Bindless set instance that can be used by Pipeline objects </summary>
			class JIMARA_API Instance : public virtual Object {};

			/// <summary>
			/// Creates or retrieves bindless "binding" of given object 
			/// <para/> Notes:
			///		<para/> 0. Calls with the same resource as a parameter will return the same object;
			///		<para/> 1. nullptr is a valid binding, although your shader may not be a fan of it;
			///		<para/> 2. Since the index association is technically only alive till the moment Binding goes out of scope,
			///				it's adviced to keep alive Binding objects for the whole duration of the frame they are being used throught.
			/// </summary>
			/// <param name="object"> Object to associate with some index </param>
			/// <returns> Object to array index association token </returns>
			virtual Reference<Binding> GetBinding(DataType* object) = 0;

			/// <summary>
			/// Creates an instance of the bindless set that can be shared among pipelines
			/// </summary>
			/// <param name="maxInFlightCommandBuffers"> 
			///		Maximal number of in-flight command buffers that can simultanuously use this instance
			///		(There is no limit on the number of pipelines that use the instance with same in-flight buffer id)
			/// </param>
			/// <returns> Bindless set instance </returns>
			virtual Reference<Instance> CreateInstance(size_t maxInFlightCommandBuffers) = 0;
		};
	}
}
