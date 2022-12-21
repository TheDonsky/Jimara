#pragma once
#include "../../../../GraphicsSimulation/GraphicsSimulation.h"
#include "../../../ViewportDescriptor.h"
#include "../../ParticleBuffers.h"


namespace Jimara {
	/// <summary>
	/// A particle kernel that generates transform matrices for particles.
	/// <para/> Notes:
	///	<para/> 0. Used internally by the Particle Systems and user does not have to think too much about it;
	/// <para/> 1. Executed after ParticleSimulationStepKernel.
	/// </summary>
	class JIMARA_API ParticleInstanceBufferGenerator : public virtual GraphicsSimulation::Task {
	public:
		ParticleInstanceBufferGenerator(GraphicsSimulation::Task* simulationStep);

		virtual ~ParticleInstanceBufferGenerator();

		void SetBuffers(ParticleBuffers* buffers);

		void SetTransform(const Matrix4& transform);

		void BindViewportRanges(const ViewportDescriptor* viewport, 
			const Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* instanceBuffer, size_t startIndex,
			const Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* indirectDrawBuffer, size_t indirectIndex);

		void UnbindViewportRange(const ViewportDescriptor* viewport);

		/// <summary>
		/// Invoked by GraphicsSimulation during the graphics synch point;
		/// If a task has dependencies that have to be executed before it, this is the place to report them
		/// </summary>
		/// <param name="recordDependency"> Each task this one depends on should be reported through this callback </param>
		virtual void GetDependencies(const Callback<GraphicsSimulation::Task*>& recordDependency)const override;

	private:
		const Reference<GraphicsSimulation::Task> m_simulationStep;

		mutable SpinLock m_lock;

		Reference<ParticleBuffers> m_buffers;
		Reference<const Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> m_particleIndirectionBuffer = nullptr;
		Reference<const Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> m_particleStateBuffer = nullptr;
		Reference<const Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> m_liveParticleCountBuffer = nullptr;

		Matrix4 m_baseTransform = Math::Identity();

		std::unordered_map<Reference<const ViewportDescriptor>, size_t> m_viewportTasks;
		struct ViewportTask {
			GraphicsSimulation::Task* task = nullptr;
			Reference<GraphicsSimulation::Task> taskRef;
			Reference<const ViewportDescriptor> viewport;
			Reference<const Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> transformBuffer;
			size_t transformStartIndex = 0u;
			Reference<const Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> indirectDrawBuffer;
			size_t indirectDrawIndex = 0u;
		};
		Stacktor<ViewportTask, 1u> m_viewTasks;

		struct Helpers;
	};
}
