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
			inline Task(const ParticleKernel* kernel, SceneContext* context)
				: m_kernel(kernel), m_context(context) {
				assert(kernel != nullptr);
				assert(context != nullptr);
				m_settingsBuffer.Resize(m_kernel->TaskSettingsSize());
			}

			inline const ParticleKernel* Kernel()const { return m_kernel; }

			inline SceneContext* Context()const { return m_context; }

			template<typename Type>
			inline void SetSettings(const Type& settings) {
				std::memcpy(
					reinterpret_cast<void*>(m_settingsBuffer.Data()), 
					reinterpret_cast<const void*>(&settings), 
					Math::Min(sizeof(settings), m_settingsBuffer.Size()));
			}

			template<typename Type>
			inline Type GetSettings()const {
				Type settings = {};
				std::memcpy(
					reinterpret_cast<void*>(&settings),
					reinterpret_cast<const void*>(m_settingsBuffer.Data()),
					Math::Min(sizeof(settings), m_settingsBuffer.Size()));
				return settings;
			}

			virtual MemoryBlock Settings()const { return MemoryBlock(m_settingsBuffer.Data(), m_settingsBuffer.Size(), nullptr); }

			inline virtual void Synchronize() {}

			inline virtual void GetDependencies(const Callback<Task*>& recordDependency)const { Unused(recordDependency); }

			template<typename RecordDependencyFn>
			inline void GetDependencies(const RecordDependencyFn& recordDependency)const {
				GetDependencies(Callback<Task*>::FromCall(&recordDependency));
			}

		private:
			const Reference<const ParticleKernel> m_kernel;
			const Reference<SceneContext> m_context;
			Stacktor<uint32_t, 128> m_settingsBuffer;
		};

		class JIMARA_API Instance : public virtual Object {
		public:
			virtual void Execute(Graphics::Pipeline::CommandBufferInfo commandBufferInfo, const Task* const* tasks, size_t taskCount) = 0;
		};

		inline ParticleKernel(size_t taskSettingsSize) : m_taskSettingsSize(taskSettingsSize) {}

		inline size_t TaskSettingsSize()const { return m_taskSettingsSize; }

		inline virtual Reference<Instance> CreateInstance(SceneContext* context)const = 0;


	private:
		const size_t m_taskSettingsSize;
	};
}
