#include "TransientBufferSet.h"
#include "../../Core/Collections/ObjectCache.h"
#include "../../Core/Stopwatch.h"
#include <map>


namespace Jimara {
	namespace Graphics {
		struct TransientBufferSet::Helpers {
			struct BufferInstance {
				Reference<ArrayBuffer> buffer;
				SpinLock bufferLock;
			};

#pragma warning(disable: 4250)
			struct Instance 
				: public virtual TransientBufferSet
				, public virtual ObjectCache<Reference<const Object>>::StoredObject {
				const Reference<GraphicsDevice> device;
				BufferInstance buffers[MAX_RECURSION_DEPTH];

				inline Instance(GraphicsDevice* d) : device(d) { assert(device != nullptr); }
				inline virtual ~Instance() {}
			};
#pragma warning(default: 4250)

			struct Cache : public virtual ObjectCache<Reference<const Object>> {
				static Reference<Instance> Get(GraphicsDevice* device) {
					static Cache cache;
					return cache.GetCachedOrCreate(device, [&]() { return Object::Instantiate<Instance>(device); });
				}
			};

			static std::atomic_size_t& RecursionDepth() {
				static thread_local std::atomic_size_t depth = 0u;
				return depth;
			}
		};

		Reference<TransientBufferSet> TransientBufferSet::Get(GraphicsDevice* device) {
			if (device == nullptr)
				return nullptr;
			return Helpers::Cache::Get(device);
		}

		bool TransientBufferSet::LockBuffer(size_t minSize, const Callback<ArrayBuffer*>& action) {
			Helpers::Instance* set = dynamic_cast<Helpers::Instance*>(this);
			assert(set != nullptr);
			
			const size_t recursionDepth = Helpers::RecursionDepth().fetch_add(1u);

			bool success = false;
			if (recursionDepth < MAX_RECURSION_DEPTH) {
				const uint8_t index = static_cast<uint8_t>(recursionDepth);
				assert(index == recursionDepth);
				const Reference<ArrayBuffer> buffer = GetBuffer(minSize, index);
				if (buffer != nullptr) {
					success = true;
					action(buffer);
				}
			}
			else set->device->Log()->Error(
				"TransientBufferSet::LockBuffer - Recursion depth exceeded MAX_RECURSION_DEPTH of ", MAX_RECURSION_DEPTH, "! ",
				"[File: ", __FILE__, "; Line: ", __LINE__, "]");

			Helpers::RecursionDepth().fetch_sub(1u);
			return success;
		}

		size_t TransientBufferSet::RecursionDepth() { 
			return Helpers::RecursionDepth(); 
		}

		Reference<ArrayBuffer> TransientBufferSet::GetBuffer(size_t minSize, uint8_t scopeDepth) {
			Helpers::Instance* set = dynamic_cast<Helpers::Instance*>(this);
			assert(set != nullptr);
			const constexpr size_t MIN_BUFFER_SIZE = 256u;
			assert(scopeDepth < MAX_RECURSION_DEPTH);
			Reference<ArrayBuffer> buffer;
			{
				auto& instance = set->buffers[scopeDepth];
				std::unique_lock<decltype(instance.bufferLock)> lock(instance.bufferLock);
				if (instance.buffer == nullptr || instance.buffer->ObjectCount() < minSize) {
					instance.buffer = set->device->CreateArrayBuffer(1u, Math::Max(
						minSize,
						(instance.buffer != nullptr)
						? (instance.buffer->ObjectCount() << 1u)
						: MIN_BUFFER_SIZE),
						Buffer::CPUAccess::CPU_WRITE_ONLY);
				}
				buffer = instance.buffer;
			}
			if (buffer == nullptr)
				set->device->Log()->Error(
					"TransientBufferSet::GetBuffer - Failed to allocate the buffer! ",
					"[File: ", __FILE__, "; Line: ", __LINE__, "]");
			return buffer;
		}
	}
}
