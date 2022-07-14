#pragma once
namespace Jimara {
	namespace Graphics {
		class Buffer;
		class ArrayBuffer;
		class Texture;
		class TextureView;
		class TestureSampler;

	namespace Experimental {
		template<typename DataType> class BindlessSet;

		typedef BindlessSet<Buffer> ConstantBufferBindingSet;

		typedef BindlessSet<ArrayBuffer> StructuredBufferBindingSet;

		typedef BindlessSet<TextureView> TextureViewBindingSet;

		typedef BindlessSet<TestureSampler> CombinedSamplerBindingSet;
	}
	}
}
#include "../../GraphicsDevice.h"


namespace Jimara {
	namespace Graphics {
	namespace Experimental {
		template<typename DataType> 
		class JIMARA_API BindlessSet : public virtual Object {
		public:
			class Binding;

			class Instance;

			virtual Reference<Binding> GetBinding(DataType* object) = 0;

			virtual Reference<Instance> CreateInstance(size_t maxInFlightCommandBuffers) = 0;
		};

		template<typename DataType>
		class JIMARA_API BindlessSet<DataType>::Binding : public virtual Object {
		public:
			inline Binding(uint32_t index) : m_index(index) {}

			inline uint32_t Index()const { return m_index; }

			virtual DataType* BoundObject()const = 0;

		private:
			const uint32_t m_index;
		};

		template<typename DataType>
		class JIMARA_API BindlessSet<DataType>::Instance : public virtual Object {
		public:
		};
	}
	}
}
