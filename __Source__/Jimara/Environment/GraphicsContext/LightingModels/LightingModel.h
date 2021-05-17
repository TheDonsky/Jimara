#pragma once
namespace Jimara { class LightingModel; }
#include "ShaderLoader.h"
#include "../GraphicsContext.h"

namespace Jimara {
	class LightingModel : public virtual Object {
	public:
		class ViewportDescriptor : public virtual Object {
		private:
			const Reference<GraphicsContext> m_graphicsContext;

		public:
			virtual Matrix4 ViewMatrix()const = 0;

			virtual Matrix4 ProjectionMatrix(float aspect)const = 0;

			virtual Vector4 ClearColor()const = 0;

			inline GraphicsContext* Context()const { return m_graphicsContext; }

		protected:
			inline ViewportDescriptor(GraphicsContext* context) : m_graphicsContext(context) {}
		};

		virtual Reference<Graphics::ImageRenderer> CreateRenderer(const ViewportDescriptor* viewport) = 0;
	};
}
