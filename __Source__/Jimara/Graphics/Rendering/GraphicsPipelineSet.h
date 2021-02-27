#pragma once
#include "../GraphicsDevice.h"
#include "../../Core/Synch/Semaphore.h"
#include <unordered_map>
#include <thread>

namespace Jimara {
	namespace Graphics {
		/// <summary> Set of graphics pipelines, that will always execute within the same render pass on pramary command byffers from the same queue </summary>
		class GraphicsPipelineSet : public virtual Object {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="queue"> Queue, the primary command buffers are expected to target </param>
			/// <param name="renderPass"> Render pass, that will be "active", when we record the commands </param>
			/// <param name="maxInFlightCommandBuffers"> Maximal number of primary command buffers that can simultinously be using this set </param>
			GraphicsPipelineSet(DeviceQueue* queue, RenderPass* renderPass, size_t maxInFlightCommandBuffers);

			/// <summary> Virtual destructor </summary>
			virtual ~GraphicsPipelineSet();

			/// <summary>
			/// Adds pipelines to the set
			/// </summary>
			/// <param name="descriptors"> References to descriptors </param>
			/// <param name="count"> Number of descriptors </param>
			void AddPipelines(const Reference<GraphicsPipeline::Descriptor>* descriptors, size_t count);

			/// <summary>
			/// Removes pipelines to the set
			/// </summary>
			/// <param name="descriptors"> References to descriptors </param>
			/// <param name="count"> Number of descriptors </param>
			void RemovePipelines(const Reference<GraphicsPipeline::Descriptor>* descriptors, size_t count);

			/// <summary>
			/// Executes pipelines on given command buffer
			/// </summary>
			/// <param name="commandBuffer"> Command buffer to execute pipelines on </param>
			/// <param name="commandBufferId"> In-flight command buffer index </param>
			void ExecutePipelines(PrimaryCommandBuffer* commandBuffer, size_t commandBufferId);

			/// <summary>
			/// Records pipelines on secondary command buffers in parallel and stores their references sequentially in a vector of those
			/// </summary>
			/// <param name="secondaryBuffers"> List of command buffers to append recorded data to </param>
			/// <param name="commandBufferId"> In-flight command buffer index </param>
			void RecordPipelines(std::vector<Reference<SecondaryCommandBuffer>>& secondaryBuffers, size_t commandBufferId);


		private:
			/* ENVIRONMENT INFO: */

			// Queue, the commands will execute on
			const Reference<DeviceQueue> m_queue;
			
			// Render pass, that will be "active", when we record the commands
			const Reference<RenderPass> m_renderPass;
			
			// Maximal number of in-flight command buffers
			const size_t m_maxInFlightCommandBuffers;


			/* STORED PIPELINES: */

			// Data about a pipeline descriptor
			struct DescriptorData {
				// Descriptor
				Reference<GraphicsPipeline::Descriptor> descriptor;
				
				// Pipeline
				Reference<GraphicsPipeline> pipeline;
			};

			// Lock for stored pipelines
			std::mutex m_dataLock;

			// Mapping from descriptor to it's data index
			std::unordered_map<GraphicsPipeline::Descriptor*, size_t> m_dataMap;
			
			// Stored pipeline data
			std::vector<DescriptorData> m_data;


			/* WORKERS: */

			// Commands, available to workers
			enum class WorkerCommand : uint8_t {
				// Workers do nothing
				NO_OP = 0,

				// Worker threads stop execution
				QUIT = 1,

				// Worker threads fill in the default execution order
				RESET_PIPELINE_ORDER = 2,

				// Workers record pipelines in secondary command buffers
				RECORD_PIPELINES = 3,

				// Not a command; denotes how many different commands there are
				JOB_TYPE_COUNT = 4
			};

			// Current command, broadcast to the workers
			volatile WorkerCommand m_workerCommand;

			// Data per worker
			struct WorkerData {
				// Command pool
				Reference<CommandPool> pool;
				
				// Secondary command buffers per in-flight command buffers
				std::vector<Reference<SecondaryCommandBuffer>> commandBuffers;

				// Semaphore for iteration
				Semaphore semaphore;
			};

			// Data per worker thread
			std::vector<WorkerData> m_workerData;
			
			// Worker threads
			std::vector<std::thread> m_workers;

			// Semaphore, workers signal, when the job's done
			Semaphore m_workDoneSemaphore;

			// Submits job to workers and waits for execution
			void ExecuteJob(WorkerCommand command);

			// Worker thread logic
			static void WorkerThread(GraphicsPipelineSet* self, size_t threadId);

			
			/* JOB-SPECIFIC data: */

			// In-flight buffer id for the command buffers, that are currently being recorded
			volatile size_t m_inFlightBufferId;

			// Order of pipeline execution (by index)
			std::vector<size_t> m_pipelineOrder;
		};


		/// <summary>
		/// Graphics pipeline descriptor collection (Yep, you may have just one of theese per scene)
		/// </summary>
		class GraphicsObjectSet : public virtual Object {
		public:
			/// <summary> Virtual destructor </summary>
			virtual ~GraphicsObjectSet();

			/// <summary>
			/// Adds descriptors to the set
			/// Note: Keep in mind, that the descriptor will not go out of scope unless you make a manual RemovePipeline(s) call
			/// </summary>
			/// <param name="descriptors"> Descriptors references </param>
			/// <param name="count"> Number of descriptors </param>
			void AddPipelines(const Reference<GraphicsPipeline::Descriptor>* descriptors, size_t count);

			/// <summary>
			/// Removes descriptors from the set
			/// Note: Keep in mind, that the descriptor will not go out of scope unless you make a manual RemovePipeline(s) call
			/// </summary>
			/// <param name="descriptors"> Descriptors references </param>
			/// <param name="count"> Number of descriptors </param>
			void RemovePipelines(const Reference<GraphicsPipeline::Descriptor>* descriptors, size_t count);

			/// <summary>
			/// Adds a descriptor to the set
			/// Note: Keep in mind, that the descriptor will not go out of scope unless you make a manual RemovePipeline(s) call
			/// </summary>
			/// <param name="descriptors"> Descriptor reference </param>
			void AddPipeline(GraphicsPipeline::Descriptor* descriptor);

			/// <summary>
			/// Removes a descriptor from the set
			/// Note: Keep in mind, that the descriptor will not go out of scope unless you make a manual RemovePipeline(s) call
			/// </summary>
			/// <param name="descriptors"> Descriptor reference </param>
			void RemovePipeline(GraphicsPipeline::Descriptor* descriptor);

			/// <summary>
			/// Adds change listener callbacks
			/// </summary>
			/// <param name="onPipelinesAdded"> Will be invoked each time at least one descriptor gets added to the set, as well as during AddChangeCallbacks() call </param>
			/// <param name="onPipelinesRemoved"> Will be invoked each time at least one descriptor gets removed from the set, as well ad during RemoveChangeCallbacks() call and whenever this goes out of scope </param>
			void AddChangeCallbacks(
				Callback<const Reference<GraphicsPipeline::Descriptor>*, size_t, GraphicsObjectSet*> onPipelinesAdded,
				Callback<const Reference<GraphicsPipeline::Descriptor>*, size_t, GraphicsObjectSet*> onPipelinesRemoved);
			
			/// <summary>
			/// Removes change listener callbacks
			/// </summary>
			/// <param name="onPipelinesAdded"> Will be invoked each time at least one descriptor gets added to the set, as well as during AddChangeCallbacks() call </param>
			/// <param name="onPipelinesRemoved"> Will be invoked each time at least one descriptor gets removed from the set, as well ad during RemoveChangeCallbacks() call and whenever this goes out of scope </param>
			void RemoveChangeCallbacks(
				Callback<const Reference<GraphicsPipeline::Descriptor>*, size_t, GraphicsObjectSet*> onPipelinesAdded,
				Callback<const Reference<GraphicsPipeline::Descriptor>*, size_t, GraphicsObjectSet*> onPipelinesRemoved);


		private:
			// Lock for stored descriptors
			std::mutex m_dataLock;

			// Decriptor to data index map
			std::unordered_map<GraphicsPipeline::Descriptor*, size_t> m_dataMap;

			// Descriptor data collection
			std::vector<Reference<GraphicsPipeline::Descriptor>> m_data;

			// Descriptor addition callback
			EventInstance<const Reference<GraphicsPipeline::Descriptor>*, size_t, GraphicsObjectSet*> m_onPipelinesAdded;

			// Descriptor removal callback
			EventInstance<const Reference<GraphicsPipeline::Descriptor>*, size_t, GraphicsObjectSet*> m_onPipelinesRemoved;
		};
	}
}
