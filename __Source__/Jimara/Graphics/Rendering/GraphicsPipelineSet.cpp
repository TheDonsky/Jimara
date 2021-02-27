#include "GraphicsPipelineSet.h"

namespace Jimara {
	namespace Graphics {
		GraphicsPipelineSet::GraphicsPipelineSet(DeviceQueue* queue, RenderPass* renderPass, size_t maxInFlightCommandBuffers, size_t threadCount)
			: m_queue(queue), m_renderPass(renderPass), m_maxInFlightCommandBuffers(maxInFlightCommandBuffers)
			, m_workerCommand(WorkerCommand::NO_OP), m_workerData(threadCount)
			, m_inFlightBufferId(0) {
			for (size_t i = 0; i < m_workerData.size(); i++)
				m_workers[i] = std::thread(GraphicsPipelineSet::WorkerThread, this, i);
		}

		GraphicsPipelineSet::~GraphicsPipelineSet() {
			ExecuteJob(WorkerCommand::QUIT);
			for (size_t i = 0; i < m_workers.size(); i++)
				m_workers[i].join();
		}

		void GraphicsPipelineSet::AddPipelines(const Reference<GraphicsPipeline::Descriptor>* descriptors, size_t count) {
			if (descriptors == nullptr || count <= 0) return;
			std::unique_lock<std::mutex> lock(m_dataLock);
			bool pipelineSetChanged = false;
			for (size_t i = 0; i < count; i++) {
				const Reference<GraphicsPipeline::Descriptor>& desc = descriptors[i];
				if (desc == nullptr) continue;
				else if (m_dataMap.find(desc) != m_dataMap.end()) continue;
				
				DescriptorData data;
				data.descriptor = desc;

				m_dataMap[desc] = m_data.size();
				m_data.push_back(data);
				pipelineSetChanged = true;
			}
			if (pipelineSetChanged) m_pipelineOrder.clear();
		}

		void GraphicsPipelineSet::RemovePipelines(const Reference<GraphicsPipeline::Descriptor>* descriptors, size_t count) {
			if (descriptors == nullptr || count <= 0) return;
			std::unique_lock<std::mutex> lock(m_dataLock);
			bool pipelineSetChanged = false;
			for (size_t i = 0; i < count; i++) {
				const Reference<GraphicsPipeline::Descriptor>& desc = descriptors[i];
				if (desc == nullptr) continue;
				std::unordered_map<GraphicsPipeline::Descriptor*, size_t>::iterator it = m_dataMap.find(desc);
				if (it == m_dataMap.end()) continue;
				const size_t index = it->second;
				DescriptorData& data = m_data[index];
				m_dataMap.erase(it);
				const size_t lastIndex = (m_data.size() - 1);
				if (index < lastIndex) {
					data = m_data[lastIndex];
					m_dataMap[data.descriptor] = index;
				}
				m_data.pop_back();
				pipelineSetChanged = true;
			}
			if (pipelineSetChanged) m_pipelineOrder.clear();
		}

		void GraphicsPipelineSet::ExecutePipelines(PrimaryCommandBuffer* commandBuffer, size_t commandBufferId) {
			static thread_local std::vector<Reference<SecondaryCommandBuffer>> buffers;
			RecordPipelines(buffers, commandBufferId);
			for (size_t i = 0; i < buffers.size(); i++)
				commandBuffer->ExecuteCommands(buffers[i]);
			buffers.clear();
		}

		void GraphicsPipelineSet::RecordPipelines(std::vector<Reference<SecondaryCommandBuffer>>& secondaryBuffers, size_t commandBufferId) {
			if (m_pipelineOrder.size() < m_data.size()) {
				m_pipelineOrder.resize(m_data.size());
				ExecuteJob(WorkerCommand::RESET_PIPELINE_ORDER);
			}
			m_inFlightBufferId = commandBufferId;
			ExecuteJob(WorkerCommand::RECORD_PIPELINES);
			for (size_t i = 0; i < m_workerData.size(); i++)
				secondaryBuffers.push_back(m_workerData[i].commandBuffers[commandBufferId]);
		}

		namespace {
			typedef bool(*JobFn)(GraphicsPipelineSet* self, size_t threadId);
		}

		void GraphicsPipelineSet::ExecuteJob(WorkerCommand command) {
			m_workerCommand = command;
			for (size_t i = 0; i < m_workerData.size(); i++)
				m_workerData[i].semaphore.post();
			m_workDoneSemaphore.wait(m_workerData.size());
		}

		void GraphicsPipelineSet::WorkerThread(GraphicsPipelineSet* self, size_t threadId) {
			static const JobFn* JOBS = []() -> JobFn* {
				const uint8_t JOB_TYPE_COUNT = static_cast<uint8_t>(WorkerCommand::JOB_TYPE_COUNT);
				static JobFn jobs[JOB_TYPE_COUNT];
				
				// NO_OP Job: Does nothing
				const JobFn NO_OP = [](GraphicsPipelineSet*, size_t) -> bool { return false; };
				for (uint8_t i = 0; i < JOB_TYPE_COUNT; i++) jobs[i] = NO_OP;

				// QUIT Job: Instructs workers to exit threads
				jobs[static_cast<uint8_t>(WorkerCommand::QUIT)] = [](GraphicsPipelineSet*, size_t) -> bool { return true; };

				// <Used by some jobs; given thread id and set pointer, extracts "Target pipeline range">
				static auto extractRange = [](GraphicsPipelineSet* self, size_t threadId) -> std::pair<size_t, size_t> {
					const size_t threadCount = self->m_workerData.size();
					const size_t pipelineCount = self->m_pipelineOrder.size();
					const size_t pipelinesPerWorker = (pipelineCount + threadCount - 1) / threadCount;
					const size_t first = pipelinesPerWorker * threadId;
					return std::make_pair(first, (threadId < (threadCount - 1)) ? (first + pipelinesPerWorker) : pipelineCount);
				};

				// RESET_PIPELINE_ORDER Job: Sets pipeline order the same as the data array
				jobs[static_cast<uint8_t>(WorkerCommand::RESET_PIPELINE_ORDER)] = [](GraphicsPipelineSet* self, size_t threadId) -> bool {
					std::pair<size_t, size_t> range = extractRange(self, threadId);
					for (size_t i = range.first; i < range.second; i++)
						self->m_pipelineOrder[i] = i;
					return false;
				};

				// RECORD_PIPELINES Job: Records pipeline execution on secondary command buffers
				jobs[static_cast<uint8_t>(WorkerCommand::RECORD_PIPELINES)] = [](GraphicsPipelineSet* self, size_t threadId) -> bool {
					WorkerData& worker = self->m_workerData[threadId];
					if (worker.commandBuffers.size() < self->m_maxInFlightCommandBuffers) {
						if (worker.pool == nullptr) worker.pool = self->m_queue->CreateCommandPool();
						worker.commandBuffers = worker.pool->CreateSecondaryCommandBuffers(self->m_maxInFlightCommandBuffers);
					}
					Pipeline::CommandBufferInfo info(worker.commandBuffers[self->m_inFlightBufferId], self->m_inFlightBufferId);
					info.commandBuffer->Reset();
					info.commandBuffer->BeginRecording();
					std::pair<size_t, size_t> range = extractRange(self, threadId);
					for (size_t i = range.first; i < range.second; i++) {
						DescriptorData& data = self->m_data[self->m_pipelineOrder[i]];
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
					return false;
				};

				return jobs;
			}();

			while (true) {
				self->m_workerData[threadId].semaphore.wait();
				bool shouldExit = JOBS[static_cast<uint8_t>(self->m_workerCommand)](self, threadId);
				self->m_workDoneSemaphore.post();
				if (shouldExit) break;
			}
		}





		GraphicsObjectSet::~GraphicsObjectSet() {
			m_onPipelinesRemoved(m_data.data(), m_data.size(), this);
		}

		void GraphicsObjectSet::AddPipelines(const Reference<GraphicsPipeline::Descriptor>* descriptors, size_t count) {
			if (descriptors == nullptr || count <= 0) return;
			std::unique_lock<std::mutex> lock(m_dataLock);
			size_t numAdded = 0;
			for (size_t i = 0; i < count; i++) {
				const Reference<GraphicsPipeline::Descriptor>& desc = descriptors[i];
				if (desc == nullptr) continue;
				else if (m_dataMap.find(desc) != m_dataMap.end()) continue;
				m_dataMap[desc] = m_data.size();
				m_data.push_back(desc);
				numAdded++;
			}
			if (numAdded > 0)
				m_onPipelinesAdded(&m_data[m_data.size() - numAdded], numAdded, this);
		}

		void GraphicsObjectSet::RemovePipelines(const Reference<GraphicsPipeline::Descriptor>* descriptors, size_t count) {
			if (descriptors == nullptr || count <= 0) return;
			std::unique_lock<std::mutex> lock(m_dataLock);
			size_t numRemoved = 0;
			for (size_t i = 0; i < count; i++) {
				const Reference<GraphicsPipeline::Descriptor>& desc = descriptors[i];
				if (desc == nullptr) continue;
				std::unordered_map<GraphicsPipeline::Descriptor*, size_t>::iterator it = m_dataMap.find(desc);
				if (it == m_dataMap.end()) continue;
				const size_t index = it->second;
				Reference<GraphicsPipeline::Descriptor>& reference = m_data[index];
				m_dataMap.erase(it);
				numRemoved++;
				const size_t lastIndex = (m_data.size() - numRemoved);
				if (index < lastIndex) {
					std::swap(reference, m_data[lastIndex]);
					m_dataMap[reference] = index;
				}
			}
			if (numRemoved > 0) {
				const size_t sizeLeft = (m_data.size() - numRemoved);
				m_onPipelinesRemoved(&m_data[sizeLeft], numRemoved, this);
				m_data.resize(sizeLeft);
			}
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
			onPipelinesAdded(m_data.data(), m_data.size(), this);
			((Event<const Reference<GraphicsPipeline::Descriptor>*, size_t, GraphicsObjectSet*>&)m_onPipelinesAdded) += onPipelinesAdded;
			((Event<const Reference<GraphicsPipeline::Descriptor>*, size_t, GraphicsObjectSet*>&)m_onPipelinesRemoved) += onPipelinesRemoved;
		}

		void GraphicsObjectSet::RemoveChangeCallbacks(
			Callback<const Reference<GraphicsPipeline::Descriptor>*, size_t, GraphicsObjectSet*> onPipelinesAdded,
			Callback<const Reference<GraphicsPipeline::Descriptor>*, size_t, GraphicsObjectSet*> onPipelinesRemoved) {
			onPipelinesRemoved(m_data.data(), m_data.size(), this);
			((Event<const Reference<GraphicsPipeline::Descriptor>*, size_t, GraphicsObjectSet*>&)m_onPipelinesAdded) -= onPipelinesAdded;
			((Event<const Reference<GraphicsPipeline::Descriptor>*, size_t, GraphicsObjectSet*>&)m_onPipelinesRemoved) -= onPipelinesRemoved;
		}
	}
}
