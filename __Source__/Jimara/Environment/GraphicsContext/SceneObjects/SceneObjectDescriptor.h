#pragma once
#include "../../../Data/Material.h"
#include "../../../Math/Math.h"


namespace Jimara {
	/// <summary>
	/// Simple descriptor of a scene object
	/// </summary>
	class SceneObjectDescriptor : public virtual Object, public virtual Graphics::ShaderResourceBindings::ShaderResourceBindingSet {
	private:
		// Shader class (Because of some dependencies, this can not change, threfore we have it kind of hard coded here)
		const Reference<Graphics::ShaderClass> m_shaderClass;

	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="shaderClass"> Shader class (Because of some dependencies, this can not change, threfore we have it kind of hard coded here) </param>
		inline SceneObjectDescriptor(Graphics::ShaderClass* shaderClass) : m_shaderClass(shaderClass) {}

		/// <summary> Shader class to use for rendering </summary>
		inline Graphics::ShaderClass* ShaderClass()const { return m_shaderClass; }

		/// <summary> Boundaries, covering the entire volume of the scene object (useful for culling and sorting) </summary>
		virtual AABB Bounds()const = 0;


		/// <summary> Number of vertex buffers, used by the vertex shader (tied to material; should not change) </summary>
		virtual size_t VertexBufferCount()const = 0;

		/// <summary>
		/// Vertex buffer by index
		/// </summary>
		/// <param name="index"> Vertex buffer index </param>
		/// <returns> Index'th vertex buffer </returns>
		virtual Reference<Graphics::VertexBuffer> VertexBuffer(size_t index)const = 0;


		/// <summary> 
		/// Number of instance buffers, used by the vertex shader 
		/// (basically, vertex buffers that are delivered per-instance, instead of per vertex. tied to material; should not change) 
		/// </summary>
		virtual size_t InstanceBufferCount()const = 0;

		/// <summary>
		/// Instance buffer by index
		/// </summary>
		/// <param name="index"> Instance buffer index </param>
		/// <returns> Index'th instance buffer </returns>
		virtual Reference<Graphics::InstanceBuffer> InstanceBuffer(size_t index)const = 0;


		/// <summary> Index buffer </summary>
		virtual Graphics::ArrayBufferReference<uint32_t> IndexBuffer()const = 0;

		/// <summary> Number of indices to use from index buffer (helps when we want to reuse the index buffer object even when we change geometry or something) </summary>
		virtual size_t IndexCount()const = 0;

		/// <summary> Number of instances to draw (by ignoring some of the instance buffer members, we can mostly vary instance count without any reallocation) </summary>
		virtual size_t InstanceCount()const = 0;
	};
}
