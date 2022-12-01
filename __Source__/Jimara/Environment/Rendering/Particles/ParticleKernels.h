#pragma once
#include "ParticleBuffers.h"


namespace Jimara {
	class JIMARA_API ParticleInitializationKernel : public virtual GraphicsSimulation::Kernel {
	public:
		class JIMARA_API Task : public virtual GraphicsSimulation::Task {
		public:
			typedef Function<Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding*, const ParticleBuffers::BufferId*> BufferSearchFn;

			void SetBuffers(ParticleBuffers* buffers);

			virtual void GetDependencies(const Callback<GraphicsSimulation::Task*>& recordDependency)const override;

		protected:
			virtual void SetBuffers(
				Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* indirectionBuffer,
				Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* liveParticleCountBuffer,
				const BufferSearchFn& findBuffer) = 0;

		private:
			mutable SpinLock m_dependencyLock;
			Stacktor<Reference<GraphicsSimulation::Task>, 4u> m_dependencies;
		};

		virtual Reference<Task> CreateTask(SceneContext* context)const = 0;

		template<typename KernelType>
		Reference<Object> CreateInstanceAttribute(const KernelType* instance) {
			return CreateInstanceAttribute(instance, TypeId::Of<KernelType>());
		}

		class JIMARA_API Set : public virtual Object {
		public:
			inline virtual ~Set() {}

			size_t Size()const;

			std::pair<const ParticleInitializationKernel*, TypeId> operator[](size_t index)const;

			const constexpr size_t InvalidIndex()const { return ~size_t(0u); }

			size_t operator[](const ParticleInitializationKernel* kernel)const;

			size_t operator[](const TypeId& type)const;

		private:
			const Reference<const Object> m_data;

			inline Set(const Object* data) : m_data(data) {}
			friend class ParticleInitializationKernel;
		};

		static Reference<Set> All();

	private:
		Reference<Object> CreateInstanceAttribute(const ParticleInitializationKernel* instance, TypeId type);
	};

	class JIMARA_API ParticleTimestepKernel : public virtual GraphicsSimulation::Kernel {
	public:
		class JIMARA_API Task : public virtual GraphicsSimulation::Task {
		public:
			virtual void SetBuffers(ParticleBuffers* buffers) = 0;

			virtual void GetDependencies(const Callback<GraphicsSimulation::Task*>& recordDependency)const override;

		private:
			Reference<GraphicsSimulation::Task> m_spawningStep;
			friend class ParticleTimestepKernel;
		};

		template<typename KernelType>
		Reference<Object> CreateInstanceAttribute(const KernelType* instance) {
			return CreateInstanceAttribute(instance, TypeId::Of<KernelType>());
		}

		Reference<Task> CreateTask(GraphicsSimulation::Task* spawningStep);

	protected:
		virtual Reference<Task> CreateTask(SceneContext* context)const = 0;

	public:
		class JIMARA_API Set : public virtual Object {
		public:
			inline virtual ~Set() {}

			size_t Size()const;

			std::pair<const ParticleTimestepKernel*, TypeId> operator[](size_t index)const;

			const constexpr size_t InvalidIndex()const { return ~size_t(0u); }

			size_t operator[](const ParticleTimestepKernel* kernel)const;

			size_t operator[](const TypeId& type)const;

		private:
			const Reference<const Object> m_data;

			inline Set(const Object* data) : m_data(data) {}
			friend class ParticleTimestepKernel;
		};

		static Reference<Set> All();

	private:
		Reference<Object> CreateInstanceAttribute(const ParticleTimestepKernel* instance, TypeId type);
	};
}
