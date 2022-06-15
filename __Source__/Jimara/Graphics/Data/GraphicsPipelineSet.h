#pragma once
#include "../GraphicsDevice.h"
#include "../../Core/Collections/ThreadBlock.h"
#include "../../Core/Collections/ObjectSet.h"
#include <unordered_map>

namespace Jimara {
	namespace Graphics {
		/// <summary> Set of graphics pipelines, that will always execute within the same render pass on pramary command byffers from the same queue </summary>
		class JIMARA_API GraphicsPipelineSet : public virtual Object {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="queue"> Queue, the primary command buffers are expected to target </param>
			/// <param name="renderPass"> Render pass, that will be "active", when we record the commands </param>
			/// <param name="maxInFlightCommandBuffers"> Maximal number of primary command buffers that can simultinously be using this set </param>
			/// <param name="threadCount"> Number of recording threads </param>
			GraphicsPipelineSet(DeviceQueue* queue, RenderPass* renderPass, size_t maxInFlightCommandBuffers, size_t threadCount = std::thread::hardware_concurrency());

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
			/// <param name="targetFrameBuffer"> Frame buffer, the render pass is executed on </param>
			/// <param name="environmentPipeline"> Shared environment pipeline </param>
			void ExecutePipelines(PrimaryCommandBuffer* commandBuffer, size_t commandBufferId, FrameBuffer* targetFrameBuffer, Pipeline* environmentPipeline);

			/// <summary>
			/// Records pipelines on secondary command buffers in parallel and stores their references sequentially in a vector of those
			/// </summary>
			/// <param name="secondaryBuffers"> List of command buffers to append recorded data to </param>
			/// <param name="commandBufferId"> In-flight command buffer index </param>
			/// <param name="targetFrameBuffer"> Frame buffer, the render pass is executed on </param>
			/// <param name="environmentPipeline"> Shared environment pipeline </param>
			void RecordPipelines(std::vector<Reference<SecondaryCommandBuffer>>& secondaryBuffers, size_t commandBufferId, FrameBuffer* targetFrameBuffer, Pipeline* environmentPipeline);


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
				mutable Reference<GraphicsPipeline> pipeline;

				// Constructor
				inline DescriptorData(GraphicsPipeline::Descriptor* desc = nullptr) : descriptor(desc) {}
			};

			// Lock for stored pipelines
			std::mutex m_dataLock;
			
			// Stored pipeline data
			ObjectSet<GraphicsPipeline::Descriptor, DescriptorData> m_data;


			/* WORKERS: */

			// Commands, available to workers
			enum class WorkerCommand : uint8_t {
				// Workers do nothing
				NO_OP = 0,

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
			};

			// Data per worker thread
			std::vector<WorkerData> m_workerData;
			
			// Thread block that executes actual commands
			ThreadBlock m_threadBlock;

			// Submits job to workers and waits for execution
			void ExecuteJob(WorkerCommand command);

			
			/* JOB-SPECIFIC data: */

			// In-flight buffer id for the command buffers, that are currently being recorded
			volatile size_t m_inFlightBufferId;

			// Order of pipeline execution (by index)
			std::vector<size_t> m_pipelineOrder;

			// m_environmentPipeline needs to be accessed by one thread at a time, so this is for synchronisation here
			std::mutex m_sharedPipelineAccessLock;

			// Frame buffer, needed during pipeline recording
			volatile FrameBuffer* m_activeFrameBuffer;

			// Environment pipeline needed during pipeline recording
			volatile Pipeline* m_environmentPipeline;
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

			/// <summary>
			/// Gets all currently stored pipelines (not thread-safe)
			/// </summary>
			/// <param name="descriptors"> Reference to the pointer to store descriptor list at </param>
			/// <param name="count"> Reference to the number to assign the descriptosr count to </param>
			void GetAllPipelines(const Reference<GraphicsPipeline::Descriptor>*& descriptors, size_t& count);


		private:
			// Lock for stored descriptors
			std::mutex m_dataLock;

			// Descriptor data collection
			ObjectSet<GraphicsPipeline::Descriptor> m_data;

			// Descriptor addition callback
			EventInstance<const Reference<GraphicsPipeline::Descriptor>*, size_t, GraphicsObjectSet*> m_onPipelinesAdded;

			// Descriptor removal callback
			EventInstance<const Reference<GraphicsPipeline::Descriptor>*, size_t, GraphicsObjectSet*> m_onPipelinesRemoved;
		};
	}
}
