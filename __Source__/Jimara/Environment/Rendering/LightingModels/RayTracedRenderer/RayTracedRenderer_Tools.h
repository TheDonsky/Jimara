#pragma once
#include "RayTracedRenderer.h"


namespace Jimara {
	struct RayTracedRenderer::Tools {
		class SharedData;
		class SharedDataManager;
		class RasterPass;
		class RayTracedPass;
		class Renderer;
	};

	class RayTracedRenderer::Tools::SharedData {
	public:
		SharedData(SharedData&& other)noexcept;
		~SharedData();

		operator bool()const;

		inline Graphics::TextureView* PrimitiveRecordIdBuffer()const { return m_primitiveRecordIdBuffer; }

		inline Graphics::TextureView* TargetColorTexture()const { return m_targetColorTexture; }

		inline Graphics::TextureView* TargetDepthTexture()const { return m_targetDepthTexture; }

	private:
		Reference<Graphics::TextureView> m_primitiveRecordIdBuffer;
		Reference<Graphics::TextureView> m_targetColorTexture;
		Reference<Graphics::TextureView> m_targetDepthTexture;

		friend class SharedDataManager;
		inline SharedData() {}
	};

	class RayTracedRenderer::Tools::SharedDataManager : public virtual Object {
	public:
		static Reference<SharedDataManager> Create(const RayTracedRenderer* renderer, const ViewportDescriptor* viewport, LayerMask layers);

		virtual ~SharedDataManager();

		inline const RayTracedRenderer* Renderer()const { return m_renderer; }

		inline const ViewportDescriptor* Viewport()const { return m_viewport; }

		inline const LayerMask& Layers()const { return m_layerMask; }

		SharedData StartPass(RenderImages* images);

	private:
		const Reference<const RayTracedRenderer> m_renderer;
		const Reference<const ViewportDescriptor> m_viewport;
		const LayerMask m_layerMask;
		const Reference<Object> m_additionalData;
		
		struct Helpers;
		SharedDataManager(const RayTracedRenderer* renderer, const ViewportDescriptor* viewport, LayerMask layers, Object* additionalData);
	};

	class RayTracedRenderer::Tools::RasterPass : public virtual Object {
	public:
		virtual ~RasterPass();

		static Reference<RasterPass> Create(const RayTracedRenderer* renderer, const ViewportDescriptor* viewport, LayerMask layers);

		void Render(Graphics::InFlightBufferInfo commandBufferInfo, const SharedData& data);

		void GetDependencies(Callback<JobSystem::Job*> report);

	private:
		// Constructor can only be invoked internally..
		RasterPass();

		// Private stuff resides in-here
		struct Helpers;
	};

	class RayTracedRenderer::Tools::RayTracedPass : public virtual Object {
	public:
		virtual ~RayTracedPass();

		static Reference<RayTracedPass> Create(const RayTracedRenderer* renderer, const ViewportDescriptor* viewport, LayerMask layers);

		void Render(Graphics::InFlightBufferInfo commandBufferInfo, const SharedData& data);

		void GetDependencies(Callback<JobSystem::Job*> report);

	private:
		// Constructor can only be invoked internally..
		RayTracedPass();

		// Private stuff resides in-here
		struct Helpers;
	};
}
