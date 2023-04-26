#include "GraphicsPipelineSet.h"

/*
namespace Jimara {
	namespace Graphics {
		GraphicsPipelineSet::GraphicsPipelineSet(DeviceQueue* queue, RenderPass* renderPass, size_t maxInFlightCommandBuffers, size_t threadCount)
			: m_queue(queue), m_renderPass(renderPass), m_maxInFlightCommandBuffers(maxInFlightCommandBuffers)
			, m_workerCommand(WorkerCommand::NO_OP), m_workerData(threadCount)
			, m_inFlightBufferId(0)
			, m_activeFrameBuffer(nullptr), m_environmentPipeline(nullptr) {}

		GraphicsPipelineSet::~GraphicsPipelineSet() {}

		void GraphicsPipelineSet::AddPipelines(const Reference<GraphicsPipeline::Descriptor>* descriptors, size_t count) {
			if (descriptors == nullptr || count <= 0) return;
			std::unique_lock<std::mutex> lock(m_dataLock);
			m_data.Add(descriptors, count, [&](const DescriptorData*, size_t numAdded) {
				if (numAdded > 0) m_pipelineOrder.clear();
				});
		}

		void GraphicsPipelineSet::RemovePipelines(const Reference<GraphicsPipeline::Descriptor>* descriptors, size_t count) {
			if (descriptors == nullptr || count <= 0) return;
			std::unique_lock<std::mutex> lock(m_dataLock);
			m_data.Remove(descriptors, count, [&](const DescriptorData*, size_t numRemoved) {
				if (numRemoved > 0) m_pipelineOrder.clear();
				});
		}

		void GraphicsPipelineSet::ExecutePipelines(PrimaryCommandBuffer* commandBuffer, size_t commandBufferId, FrameBuffer* targetFrameBuffer, Pipeline* environmentPipeline) {
			static thread_local std::vector<Reference<SecondaryCommandBuffer>> buffers;
			RecordPipelines(buffers, commandBufferId, targetFrameBuffer, environmentPipeline);
			for (size_t i = 0; i < buffers.size(); i++)
				commandBuffer->ExecuteCommands(buffers[i]);
			buffers.clear();
		}

		void GraphicsPipelineSet::RecordPipelines(
			std::vector<Reference<SecondaryCommandBuffer>>& secondaryBuffers, size_t commandBufferId, FrameBuffer* targetFrameBuffer, Pipeline* environmentPipeline) {
			std::unique_lock<std::mutex> lock(m_dataLock);
			if (m_pipelineOrder.size() < m_data.Size()) {
				m_pipelineOrder.resize(m_data.Size());
				ExecuteJob(WorkerCommand::RESET_PIPELINE_ORDER);
			}
			m_inFlightBufferId = commandBufferId;
			m_activeFrameBuffer = targetFrameBuffer;
			m_environmentPipeline = environmentPipeline;
			ExecuteJob(WorkerCommand::RECORD_PIPELINES);
			for (size_t i = 0; i < m_workerData.size(); i++)
				secondaryBuffers.push_back(m_workerData[i].commandBuffers[commandBufferId]);
		}

		namespace {
			typedef void(*JobFn)(GraphicsPipelineSet* self, size_t threadId);
		}

		void GraphicsPipelineSet::ExecuteJob(WorkerCommand command) {
			static const JobFn* JOBS = []() -> JobFn* {
				const uint8_t JOB_TYPE_COUNT = static_cast<uint8_t>(WorkerCommand::JOB_TYPE_COUNT);
				static JobFn jobs[JOB_TYPE_COUNT];

				// NO_OP Job: Does nothing
				const JobFn NO_OP = [](GraphicsPipelineSet*, size_t) {};
				for (uint8_t i = 0; i < JOB_TYPE_COUNT; i++) jobs[i] = NO_OP;

				// <Used by some jobs; given thread id and set pointer, extracts "Target pipeline range">
				static auto extractRange = [](GraphicsPipelineSet* self, size_t threadId) -> std::pair<size_t, size_t> {
					const size_t threadCount = self->m_workerData.size();
					const size_t pipelineCount = self->m_pipelineOrder.size();
					const size_t pipelinesPerWorker = (pipelineCount + threadCount - 1) / threadCount;
					const size_t first = pipelinesPerWorker * threadId;
					const size_t last = first + pipelinesPerWorker;
					return std::make_pair(first, (last < pipelineCount) ? last : pipelineCount);
				};

				// RESET_PIPELINE_ORDER Job: Sets pipeline order the same as the data array
				jobs[static_cast<uint8_t>(WorkerCommand::RESET_PIPELINE_ORDER)] = [](GraphicsPipelineSet* self, size_t threadId) {
					std::pair<size_t, size_t> range = extractRange(self, threadId);
					for (size_t i = range.first; i < range.second; i++)
						self->m_pipelineOrder[i] = i;
				};

				// RECORD_PIPELINES Job: Records pipeline execution on secondary command buffers
				jobs[static_cast<uint8_t>(WorkerCommand::RECORD_PIPELINES)] = [](GraphicsPipelineSet* self, size_t threadId) {
					WorkerData& worker = self->m_workerData[threadId];
					if (worker.commandBuffers.size() < self->m_maxInFlightCommandBuffers) {
						if (worker.pool == nullptr) worker.pool = self->m_queue->CreateCommandPool();
						worker.commandBuffers = worker.pool->CreateSecondaryCommandBuffers(self->m_maxInFlightCommandBuffers);
					}
					SecondaryCommandBuffer* commandBuffer = worker.commandBuffers[self->m_inFlightBufferId];
					InFlightBufferInfo info(commandBuffer, self->m_inFlightBufferId);
					info.commandBuffer->Reset();
					commandBuffer->BeginRecording(self->m_renderPass, (FrameBuffer*)self->m_activeFrameBuffer);
					std::pair<size_t, size_t> range = extractRange(self, threadId);
					{
						Pipeline* environment = ((Pipeline*)self->m_environmentPipeline);
						if (environment != nullptr) {
							std::unique_lock<std::mutex> lock(self->m_sharedPipelineAccessLock);
							environment->Execute(info);
						}
					}
					for (size_t i = range.first; i < range.second; i++) {
						const DescriptorData& data = self->m_data[self->m_pipelineOrder[i]];
						if (data.pipeline == nullptr) {
							data.pipeline = self->m_renderPass->CreateGraphicsPipeline(data.descriptor, self->m_maxInFlightCommandBuffers);
							if (data.pipeline == nullptr) {
								self->m_renderPass->Device()->Log()->Error("GraphicsPipelineSet::RECORD_PIPELINES - Failed to create a pipeline");
								continue;
							}
						}
						data.pipeline->Execute(info);
					}
					info.commandBuffer->EndRecording();
				};

				return jobs;
			}();

			m_workerCommand = command;
			auto job = [](ThreadBlock::ThreadInfo info, void* self) {
				GraphicsPipelineSet* set = static_cast<GraphicsPipelineSet*>(self);
				JOBS[static_cast<uint8_t>(set->m_workerCommand)](set, info.threadId);
			};
			m_threadBlock.Execute(m_workerData.size(), this, Callback<ThreadBlock::ThreadInfo, void*>(job));
		}





		GraphicsObjectSet::~GraphicsObjectSet() {
			m_onPipelinesRemoved(m_data.Data(), m_data.Size(), this);
		}

		void GraphicsObjectSet::AddPipelines(const Reference<GraphicsPipeline::Descriptor>* descriptors, size_t count) {
			if (descriptors == nullptr || count <= 0) return;
			std::unique_lock<std::mutex> lock(m_dataLock);
			m_data.Add(descriptors, count, [&](const Reference<GraphicsPipeline::Descriptor>* added, size_t numAdded) {
				if (numAdded > 0) m_onPipelinesAdded(added, numAdded, this);
				});
		}

		void GraphicsObjectSet::RemovePipelines(const Reference<GraphicsPipeline::Descriptor>* descriptors, size_t count) {
			if (descriptors == nullptr || count <= 0) return;
			std::unique_lock<std::mutex> lock(m_dataLock);
			m_data.Remove(descriptors, count, [&](const Reference<GraphicsPipeline::Descriptor>* removed, size_t numRemoved) {
				if (numRemoved > 0) m_onPipelinesRemoved(removed, numRemoved, this);
				});
		}

		void GraphicsObjectSet::AddPipeline(GraphicsPipeline::Descriptor* descriptor) {
			const Reference<GraphicsPipeline::Descriptor> desc(descriptor);
			AddPipelines(&desc, 1u);
		}

		void GraphicsObjectSet::RemovePipeline(GraphicsPipeline::Descriptor* descriptor) {
			const Reference<GraphicsPipeline::Descriptor> desc(descriptor);
			RemovePipelines(&desc, 1u);
		}

		void GraphicsObjectSet::GraphicsObjectSet::AddChangeCallbacks(
			Callback<const Reference<GraphicsPipeline::Descriptor>*, size_t, GraphicsObjectSet*> onPipelinesAdded,
			Callback<const Reference<GraphicsPipeline::Descriptor>*, size_t, GraphicsObjectSet*> onPipelinesRemoved) {
			std::unique_lock<std::mutex> lock(m_dataLock);
			onPipelinesAdded(m_data.Data(), m_data.Size(), this);
			((Event<const Reference<GraphicsPipeline::Descriptor>*, size_t, GraphicsObjectSet*>&)m_onPipelinesAdded) += onPipelinesAdded;
			((Event<const Reference<GraphicsPipeline::Descriptor>*, size_t, GraphicsObjectSet*>&)m_onPipelinesRemoved) += onPipelinesRemoved;
		}

		void GraphicsObjectSet::RemoveChangeCallbacks(
			Callback<const Reference<GraphicsPipeline::Descriptor>*, size_t, GraphicsObjectSet*> onPipelinesAdded,
			Callback<const Reference<GraphicsPipeline::Descriptor>*, size_t, GraphicsObjectSet*> onPipelinesRemoved) {
			onPipelinesRemoved(m_data.Data(), m_data.Size(), this);
			((Event<const Reference<GraphicsPipeline::Descriptor>*, size_t, GraphicsObjectSet*>&)m_onPipelinesAdded) -= onPipelinesAdded;
			((Event<const Reference<GraphicsPipeline::Descriptor>*, size_t, GraphicsObjectSet*>&)m_onPipelinesRemoved) -= onPipelinesRemoved;
		}

		void GraphicsObjectSet::GetAllPipelines(const Reference<GraphicsPipeline::Descriptor>*& descriptors, size_t& count) {
			std::unique_lock<std::mutex> lock(m_dataLock);
			descriptors = m_data.Data();
			count = m_data.Size();
		}
	}
}
*/
