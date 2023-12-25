#include "FrustrumAABBCulling.h"
#include "../../../GraphicsSimulation/CombinedGraphicsSimulationKernel.h"
#include "../../Algorithms/SegmentTree/SegmentTreeGenerationKernel.h"
#include "../../TransientBuffer.h"


namespace Jimara {
	namespace Culling {
		struct FrustrumAABBCulling::Helpers {

			struct SimulationTaskSettings {
				// Frustrum
				alignas(16) Matrix4 frustrum = {};
				
				// Buffer indices
				alignas(4) uint32_t taskThreadCount = 0u;
				alignas(4) uint32_t instanceBufferIndex = 0u;
				alignas(4) uint32_t culledBufferIndex = 0u;
				alignas(4) uint32_t countBufferIndex = 0u;

				// BBox, transform and size range offsets
				alignas(4) uint32_t bboxMinOffset = 0u;
				alignas(4) uint32_t bboxMaxOffset = 0u;
				alignas(4) uint32_t instTransformOffset = 0u;
				alignas(4) uint32_t packedClipSpaceSizeRangeOffset = 0u;

				// Buffer offsets and sizes:
				alignas(4) uint32_t culledDataOffset = 0u;
				alignas(4) uint32_t culledDataSize = 0u;
				alignas(4) uint32_t instanceInfoSize = 0u;
				alignas(4) uint32_t countValueOffset = 0u;

			};
			static_assert(sizeof(SimulationTaskSettings) == (16u * 7u));

			class KernelInstance : public virtual GraphicsSimulation::KernelInstance {
			private:
				const Reference<OS::Logger> m_log;
				const Reference<TransientBuffer> m_transientBuffer;
				const Reference<GraphicsSimulation::KernelInstance> m_frustrumCheckKernel;
				const Reference<SegmentTreeGenerationKernel> m_segmentTreeGenerator;
				const Reference<GraphicsSimulation::KernelInstance> m_reduceKernel;
				const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> m_segmentTreeBinding;

			public:
				inline KernelInstance(
					OS::Logger* log,
					TransientBuffer* transientBuffer,
					GraphicsSimulation::KernelInstance* frustrumCheckKernel,
					SegmentTreeGenerationKernel* segmentTreeGenerator,
					GraphicsSimulation::KernelInstance* reduceKernel,
					Graphics::ResourceBinding<Graphics::ArrayBuffer>* segmentTreeBinding)
					: m_log(log)
					, m_transientBuffer(transientBuffer)
					, m_frustrumCheckKernel(frustrumCheckKernel)
					, m_segmentTreeGenerator(segmentTreeGenerator)
					, m_reduceKernel(reduceKernel)
					, m_segmentTreeBinding(segmentTreeBinding) {}

				inline virtual ~KernelInstance() {}

				inline virtual void Execute(Graphics::InFlightBufferInfo commandBufferInfo, const GraphicsSimulation::Task* const* tasks, size_t taskCount)override {
					// Count total number of transforms:
					const size_t instanceCount = [&]() {
						size_t count = 0u;
						const GraphicsSimulation::Task* const* taskPtr = tasks;
						const GraphicsSimulation::Task* const* const end = taskPtr + taskCount;
						while (taskPtr < end) {
							count += (*taskPtr)->GetSettings<SimulationTaskSettings>().taskThreadCount;
							taskPtr++;
						}
						return count;
					}();

					// Update segment tree buffer:
					m_segmentTreeBinding->BoundObject() = m_transientBuffer->GetBuffer(
						sizeof(uint32_t) * SegmentTreeGenerationKernel::SegmentTreeBufferSize(instanceCount));
					if (m_segmentTreeBinding->BoundObject() == nullptr) {
						m_log->Error("FrustrumAABBCulling::Helpers::KernelInstance::Execute - Failed to retrueve transient buffer! ",
							"[File: ", __FILE__, "; Line: ", __LINE__, "]");
						return;
					}

					// Execute pipelines:
					m_frustrumCheckKernel->Execute(commandBufferInfo, tasks, taskCount);
					m_segmentTreeGenerator->Execute(commandBufferInfo, m_segmentTreeBinding->BoundObject(), instanceCount, true);
					m_reduceKernel->Execute(commandBufferInfo, tasks, taskCount);
				}
			};

			class Kernel : public virtual GraphicsSimulation::Kernel {
			public:
				inline Kernel() : GraphicsSimulation::Kernel(sizeof(SimulationTaskSettings)) {}
				static const Kernel* Instance() {
					static const Kernel instance;
					return &instance;
				}
				inline virtual Reference<GraphicsSimulation::KernelInstance> CreateInstance(SceneContext* context)const override {
					if (context == nullptr)
						return nullptr;
					auto fail = [&](const auto&... message) {
						context->Log()->Error("FrustrumAABBCulling::Helpers::Kernel::CreateInstance - ", message...);
						return nullptr;
					};

					const Reference<TransientBuffer> transientBuffer = TransientBuffer::Get(context->Graphics()->Device(), 0u);
					if (transientBuffer == nullptr)
						return fail("Failed to retrieve transient buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");

					const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> segmentTreeBufferBinding =
						Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();
					auto findSegmentTreeBufferBinding = [&](const auto& info) {
						static const constexpr std::string_view SEGMENT_TREE_BUFFER_BINDING_NAME = "segmentTreeBuffer";
						return (info.name == SEGMENT_TREE_BUFFER_BINDING_NAME) ? segmentTreeBufferBinding : nullptr;
					};
					Graphics::BindingSet::BindingSearchFunctions bindings = {};
					bindings.structuredBuffer = &findSegmentTreeBufferBinding;

					static const Graphics::ShaderClass FRUSTRUM_CHECK_KERNEL(
						"Jimara/Environment/Rendering/Culling/FrustrumAABB/FrustrumAABBCulling_FrustrumCheck");
					const Reference<GraphicsSimulation::KernelInstance> frustrumCheckKernel = 
						CombinedGraphicsSimulationKernel<SimulationTaskSettings>::Create(context, &FRUSTRUM_CHECK_KERNEL, bindings);
					if (frustrumCheckKernel == nullptr)
						return fail("Failed to create frustrum check kernel! [File: ", __FILE__, "; Line: ", __LINE__, "]");

					const Reference<SegmentTreeGenerationKernel> segmentTreeGenerator = SegmentTreeGenerationKernel::CreateUintSumKernel(
						context->Graphics()->Device(), context->Graphics()->Configuration().ShaderLoader(), 
						context->Graphics()->Configuration().MaxInFlightCommandBufferCount());
					if (segmentTreeGenerator == nullptr)
						return fail("Failed to create segment tree generator! [File: ", __FILE__, "; Line: ", __LINE__, "]");

					static const Graphics::ShaderClass REDUCE_KERNEL(
						"Jimara/Environment/Rendering/Culling/FrustrumAABB/FrustrumAABBCulling_TransformReduce");
					const Reference<GraphicsSimulation::KernelInstance> reduceKernel =
						CombinedGraphicsSimulationKernel<SimulationTaskSettings>::Create(context, &REDUCE_KERNEL, bindings);
					if (reduceKernel == nullptr)
						return fail("Failed to create reduce kernel! [File: ", __FILE__, "; Line: ", __LINE__, "]");

					return Object::Instantiate<KernelInstance>(context->Log(), transientBuffer,
						frustrumCheckKernel, segmentTreeGenerator, reduceKernel, segmentTreeBufferBinding);
				}
			};
		};

		FrustrumAABBCulling::FrustrumAABBCulling(SceneContext* context) 
			: GraphicsSimulation::Task(Helpers::Kernel::Instance(), context) {
			Configure({}, 0u, nullptr, 0u, 0u, 0u, 0u, nullptr, 0u, nullptr, 0u);
		}

		FrustrumAABBCulling::~FrustrumAABBCulling() {}

		void FrustrumAABBCulling::Configure(
			const Matrix4 & frustrum, size_t instanceCount,
			Graphics::ArrayBuffer* instanceBuffer, 
			size_t bboxMinOffset, size_t bboxMaxOffset, size_t instTransformOffset, size_t packedClipSpaceSizeRangeOffset,
			Graphics::ArrayBuffer* culledBuffer, size_t culledDataOffset,
			Graphics::ArrayBuffer* countBuffer, size_t countValueOffset) {
			
			// Make sure countValueOffset is valid
			if ((countValueOffset % sizeof(uint32_t)) != 0u) {
				Context()->Log()->Error("FrustrumAABBCulling::Configure - countValueOffset HAS TO BE multiple of ", sizeof(uint32_t), 
					"! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				instanceCount = 0u;
				instanceBuffer = nullptr;
				culledBuffer = nullptr;
				countBuffer = nullptr;
			}

			BindlessBinding transformBufferId;
			BindlessBinding culledBufferId;
			BindlessBinding countBufferId;

			// Get old indices
			{
				std::unique_lock<SpinLock> lock(m_configLock);
				transformBufferId = m_transformsBuffer;
				culledBufferId = m_culledBuffer;
				countBufferId = m_countBuffer;
			}

			// Refresh bindings
			auto update = [&](BindlessBinding& binding, Graphics::ArrayBuffer* buffer) {
				if (buffer == nullptr)
					binding = nullptr;
				else if (binding == nullptr || binding->BoundObject() != buffer)
					binding = Context()->Graphics()->Bindless().Buffers()->GetBinding(buffer);
			};
			update(transformBufferId, instanceBuffer);
			update(culledBufferId, culledBuffer);
			update(countBufferId, countBuffer);
			if (transformBufferId == nullptr || culledBufferId == nullptr || countBufferId == nullptr)
				instanceCount = 0u;

			auto toUintIndex = [](size_t offset) { return static_cast<uint32_t>(offset) >> 2u; };

			// Update settings
			{
				std::unique_lock<SpinLock> lock(m_configLock);
				m_transformsBuffer = transformBufferId;
				m_culledBuffer = culledBufferId;
				m_countBuffer = countBufferId;

				Helpers::SimulationTaskSettings settings = {};
				
				settings.frustrum = frustrum;

				settings.taskThreadCount = static_cast<uint32_t>(instanceCount);
				settings.instanceBufferIndex = (transformBufferId != nullptr) ? transformBufferId->Index() : 0u;
				settings.culledBufferIndex = (culledBufferId != nullptr) ? culledBufferId->Index() : 0u;
				settings.countBufferIndex = (countBufferId != nullptr) ? countBufferId->Index() : 0u;

				settings.bboxMinOffset = toUintIndex(bboxMinOffset);
				settings.bboxMaxOffset = toUintIndex(bboxMaxOffset);
				settings.instTransformOffset = toUintIndex(instTransformOffset);
				settings.packedClipSpaceSizeRangeOffset = toUintIndex(packedClipSpaceSizeRangeOffset);

				settings.culledDataOffset = toUintIndex(culledDataOffset);
				settings.culledDataSize = (culledBuffer == nullptr) ? 0u : toUintIndex(culledBuffer->ObjectSize());
				settings.instanceInfoSize = (instanceBuffer == nullptr) ? 0u : toUintIndex(instanceBuffer->ObjectSize());
				settings.countValueOffset = toUintIndex(countValueOffset);

				SetSettings(settings);
			}
		}
	}
}
