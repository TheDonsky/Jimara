#pragma once
#include "ParticleBuffers.h"


namespace Jimara {
	/// <summary>
	/// __TODO__: Implement this crap!
	/// </summary>
	class JIMARA_API ParticleKernel : public virtual Object {
	public:
		class JIMARA_API Task : public virtual Object {
		public:
			inline Task(const ParticleKernel* kernel, ParticleBuffers* buffers)
				: m_kernel(kernel), m_buffers(buffers) {
				assert(kernel != nullptr);
				assert(buffers != nullptr);
				m_settingsBuffer.Resize(m_kernel->TaskSettingsSize());
			}

			inline const ParticleKernel* Kernel()const { return m_kernel; }

			inline ParticleBuffers* Buffers()const { return m_buffers; }

			template<typename Type>
			inline void SetSettings(const Type& settings) {
				std::memcpy(
					reinterpret_cast<void*>(m_settingsBuffer.Data()), 
					reinterpret_cast<void*>(&settings), 
					Math::Min(sizeof(settings), m_settingsBuffer.Size()));
			}

			template<typename Type>
			inline Type GetSettings()const {
				Type settings = {};
				std::memcpy(
					reinterpret_cast<void*>(&settings),
					reinterpret_cast<void*>(m_settingsBuffer.Data()),
					Math::Min(sizeof(settings), m_settingsBuffer.Size()));
				return settings;
			}

			virtual MemoryBlock Settings()const { return MemoryBlock(m_settingsBuffer.Data(), m_settingsBuffer.Size(), nullptr); }

			inline virtual void Synchronize() {}

			inline virtual void GetDependencies(const Callback<Task*>& getDependencies)const { Unused(getDependencies); }

		private:
			const Reference<const ParticleKernel> m_kernel;
			const Reference<ParticleBuffers> m_buffers;
			Stacktor<uint32_t, 128> m_settingsBuffer;
		};

		class JIMARA_API Instance : public virtual Object {
		public:
			virtual void Execute(Graphics::Pipeline::CommandBufferInfo commandBufferInfo, const Task* const* tasks, size_t taskCount)const = 0;
		};

		inline ParticleKernel(size_t taskSettingsSize) : m_taskSettingsSize(taskSettingsSize) {}

		inline size_t TaskSettingsSize()const { return m_taskSettingsSize; }

		inline virtual Reference<Instance> CreateInstance()const = 0;


	private:
		const size_t m_taskSettingsSize;
	};
}
