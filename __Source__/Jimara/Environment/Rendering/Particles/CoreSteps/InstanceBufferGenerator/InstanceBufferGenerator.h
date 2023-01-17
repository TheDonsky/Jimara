#pragma once
#include "../SimulationStep/ParticleSimulationStepKernel.h"
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
		/// <summary>
		/// Per-instance data, generated by ParticleInstanceBufferGenerator
		/// </summary>
		struct InstanceData {
			/// <summary> Particle transform </summary>
			alignas(16) Matrix4 transform = Math::Identity();

			/// <summary> Particle color </summary>
			alignas(16) Vector4 color = Vector4(1.0f);

			/// <summary> Packed tiling and offset values (UV = MeshUV * tilingAndOffset.xy + tilingAndOffset.zw) </summary>
			alignas(16) Vector4 tilingAndOffset = Vector4(1.0f, 1.0f, 0.0f, 0.0f);
		};

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="simulationStep"> Particle simulation step </param>
		ParticleInstanceBufferGenerator(ParticleSimulationStep* simulationStep);

		/// <summary> Virtual destructor </summary>
		virtual ~ParticleInstanceBufferGenerator();

		/// <summary>
		/// Sets new particle buffers
		/// </summary>
		/// <param name="buffers"> Particle buffers to use from next update onward </param>
		void SetBuffers(ParticleBuffers* buffers);

		/// <summary>
		/// Sets transform buffer and instance/draw buffer data
		/// </summary>
		/// <param name="instanceBufferOffset"> First instance index for the particle system within the instance buffer </param>
		/// <param name="indirectDrawIndex"> Indirect draw index for the particle system </param>
		void Configure(size_t instanceBufferOffset, size_t indirectDrawIndex);

		/// <summary>
		/// Sets viewport-specific instance and indirect draw buffers
		/// </summary>
		/// <param name="viewport"> Viewport descriptor (can be nullptr) </param>
		/// <param name="instanceBuffer"> Instance buffer </param>
		/// <param name="indirectDrawBuffer"> Indirect draw buffer </param>
		void BindViewportRange(const ViewportDescriptor* viewport,
			const Graphics::ArrayBufferReference<InstanceData> instanceBuffer,
			Graphics::IndirectDrawBuffer* indirectDrawBuffer);

		/// <summary>
		/// Removes bindings associtaed with a viewport descriptor
		/// <para/> Has to be called to remove bindings previously added with BindViewportRange()
		/// </summary>
		/// <param name="viewport"> Viewport descriptor (can be nullptr) </param>
		void UnbindViewportRange(const ViewportDescriptor* viewport);

		/// <summary> "Commits" to the latest buffer change </summary>
		virtual void Synchronize()override;

		/// <summary>
		/// Invoked by GraphicsSimulation during the graphics synch point;
		/// If a task has dependencies that have to be executed before it, this is the place to report them
		/// </summary>
		/// <param name="recordDependency"> Each task this one depends on should be reported through this callback </param>
		virtual void GetDependencies(const Callback<GraphicsSimulation::Task*>& recordDependency)const override;

	private:
		// Dependency
		const Reference<ParticleSimulationStep> m_simulationStep;

		// Particle system info
		const Reference<const ParticleSystemInfo> m_systemInfo;

		// Lock for field modifications
		mutable SpinLock m_lock;

		// Bound buffers and bindings
		Reference<ParticleBuffers> m_buffers;
		Reference<ParticleBuffers> m_newBuffers;
		Reference<const Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> m_particleIndirectionBuffer = nullptr;
		Reference<const Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> m_particleStateBuffer = nullptr;
		Reference<const Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> m_liveParticleCountBuffer = nullptr;

		// Settings
		Matrix4 m_baseTransform = Math::Identity();
		size_t m_instanceStartIndex = 0u;
		size_t m_indirectDrawIndex = 0u;

		// Per viewport data
		std::unordered_map<Reference<const ViewportDescriptor>, size_t> m_viewportTasks;
		struct ViewportTask {
			GraphicsSimulation::Task* task = nullptr;
			Reference<GraphicsSimulation::Task> taskRef;
			Reference<const ViewportDescriptor> viewport;
			Reference<const Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> transformBuffer;
			Reference<const Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> indirectDrawBuffer;
		};
		Stacktor<ViewportTask, 1u> m_viewTasks;

		// Some private stuff resides here
		struct Helpers;
	};
}
