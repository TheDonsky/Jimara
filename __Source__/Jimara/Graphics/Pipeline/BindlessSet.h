#pragma once
namespace Jimara {
	namespace Graphics {
		class BindlessSet;
	}
}
#include "../Memory/Buffers.h"
#include "../Memory/Texture.h"


namespace Jimara {
	namespace Graphics {
		/// <summary>
		/// Boundless binding set for indexable binding arrays
		/// </summary>
		class JIMARA_API BindlessSet : public virtual Object {
		public:
			/// <summary>
			/// Individual boundless binding
			/// </summary>
			/// <typeparam name="Type"> Type of the bound object </typeparam>
			template<typename Type> class Binding;

			/// <summary>
			/// In order to use BoundlessSet inside a pipeline, we need to create an instance of it with the same amount of in-flight instances 
			/// </summary>
			class Instance;

			/// <summary> Binding of a constant buffer </summary>
			typedef Binding<Buffer> ConstantBufferBinding;

			/// <summary> Binding of a structured buffer </summary>
			typedef Binding<ArrayBuffer> StructuredBufferBinding;

			/// <summary> Binding of a texure sampler </summary>
			typedef Binding<TextureSampler> SamplerBinding;

			/// <summary>
			/// Creates a binding for a constant buffer
			/// <para/> Note: Depending on the backend implementation, total available number of slots may be limited
			/// </summary>
			/// <returns> Constant buffer binding </returns>
			virtual Reference<ConstantBufferBinding> AllocateConstantBufferBinding() = 0;

			/// <summary>
			/// Creates a binding for a structured buffer
			/// <para/> Note: Depending on the backend implementation, total available number of slots may be limited
			/// </summary>
			/// <returns> Constant buffer binding </returns>
			virtual Reference<StructuredBufferBinding> AllocateStructuredBufferBinding() = 0;

			/// <summary>
			/// Creates a binding for a texture sampler
			/// <para/> Note: Depending on the backend implementation, total available number of slots may be limited
			/// </summary>
			/// <returns> Constant buffer binding </returns>
			virtual Reference<SamplerBinding> AllocateSamplerBinding() = 0;

			/// <summary>
			/// Creates an instance for pipelines
			/// </summary>
			/// <param name="maxInFlightCommandBuffers"> 
			///		Maximal number of in-flight command buffers that may be using the pipeline at the same time 
			///		<para/> (This number should be same as or greater than the one used during pipeline creation)
			/// </param>
			/// <returns> BoundlessSet instance </returns>
			virtual Reference<Instance> CreateInstance(size_t maxInFlightCommandBuffers) = 0;
		};





		/// <summary>
		/// Individual boundless binding
		/// </summary>
		/// <typeparam name="Type"> Type of the bound object </typeparam>
		template<typename Type>
		class JIMARA_API BindlessSet::Binding : public virtual Object {
		public:
			/// <summary> Binding index </summary>
			inline uint32_t Index()const { return m_index; }

			/// <summary> Currently bound object </summary>
			inline Type* Object()const { return m_binding; }

			/// <summary>
			/// Binds a new object to the index
			/// <para/> Note: For performance consideration, this is not thread-safe
			/// </summary>
			/// <param name="binding"> Object to bind (can be nullptr) </param>
			inline void Bind(Type* binding) {
				if (m_binding == binding) return;
				m_binding = binding;
				Dirty();
			}

		protected:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="index"> Binding index </param>
			Binding(uint32_t index) : m_index(index) {}

			/// <summary> Invoked after SetBinding(); backends can freely take advantage of the situation </summary>
			virtual void Dirty() = 0;

		private:
			// Binding index
			const uint32_t m_index;

			// Currently bound binding
			Reference<Type> m_binding;
		};





		/// <summary>
		/// In order to use BoundlessSet inside a pipeline, we need to create an instance of it with the same amount of in-flight instances 
		/// </summary>
		class JIMARA_API BindlessSet::Instance : public virtual Object {
		public:
			/// <summary>
			/// Updates underlying data for given in-flight buffer id
			/// </summary>
			/// <param name="inFlightBufferId"> Index of the command buffer (when we have something like double/triple/quadrouple/whatever buffering; otherwise should be 0) </param>
			virtual void Update(size_t inFlightBufferId) = 0;
		};
	}
}
