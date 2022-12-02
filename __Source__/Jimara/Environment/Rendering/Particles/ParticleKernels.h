#pragma once
#include "ParticleBuffers.h"


namespace Jimara {
	class JIMARA_API ParticleInitializationTask : public virtual GraphicsSimulation::Task, public virtual Serialization::Serializable {
	public:
		class Factory;

		void SetBuffers(ParticleBuffers* buffers);

		virtual void Synchronize()final override;

		virtual void GetDependencies(const Callback<GraphicsSimulation::Task*>& recordDependency)const override;

		uint32_t SpawnedParticleCount()const;

		uint32_t ParticleBudget()const;

	protected:
		virtual void SetBuffers(
			Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* indirectionBuffer,
			Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* liveParticleCountBuffer,
			const ParticleBuffers::BufferSearchFn& findBuffer) = 0;

		virtual void UpdateSettings() = 0;

	private:
		mutable SpinLock m_dependencyLock;
		Reference<ParticleBuffers> m_buffers;
		Reference<ParticleBuffers> m_lastBuffers;
		Stacktor<Reference<GraphicsSimulation::Task>, 4u> m_dependencies;
	};

	class JIMARA_API ParticleInitializationTask::Factory : public virtual Object {
	public:
		class Set;

		template<typename TaskType> 
		inline static Reference<Factory> Of() {
			static const CreateFn createFn = [](SceneContext* context) -> Reference<ParticleInitializationTask> { 
				if (context == nullptr) return nullptr;
				const Reference<TaskType> task = Object::Instantiate<TaskType>(context);
				return Reference<ParticleInitializationTask>(task.operator->());
			};
			const Reference<Factory> instance = new Factory(createFn, TypeId::Of<TaskType>());
			instance->ReleaseRef();
			return instance;
		}

		inline virtual ~Factory() {}

		inline Reference<ParticleInitializationTask> CreateTask(SceneContext* context) const { return m_createFn(context); }

		inline TypeId TaskType()const { return m_taskType; }

	private:
		typedef Reference<ParticleInitializationTask>(*CreateFn)(SceneContext*);
		const CreateFn m_createFn;
		const TypeId m_taskType;

		inline Factory(const CreateFn& createFn, const TypeId& taskType) : m_createFn(createFn), m_taskType(taskType) {}
	};

	class JIMARA_API ParticleInitializationTask::Factory::Set : public virtual Object {
	public:
		static Reference<Set> Current();

		inline virtual ~Set() {}

		size_t Size()const;

		const Factory* operator[](size_t index)const;

		const Factory* operator[](const std::type_index& taskType)const;

		const Serialization::SerializerList::From<TypeId>* TypeSerializer()const;

	private:
		const Reference<const Object> m_data;

		inline Set(const Object* data) : m_data(data) {}
	};





	class JIMARA_API ParticleTimestepTask : public virtual GraphicsSimulation::Task, public virtual Serialization::Serializable {
	public:
		class Factory;

		void SetBuffers(ParticleBuffers* buffers);

		virtual void Synchronize()final override;

		virtual void GetDependencies(const Callback<GraphicsSimulation::Task*>& recordDependency)const override;

	protected:
		virtual void SetBuffers(const ParticleBuffers::BufferSearchFn& findBuffer) = 0;

		virtual void UpdateSettings() = 0;

	private:
		Reference<ParticleBuffers> m_buffers;
		Reference<ParticleBuffers> m_lastBuffers;
		Reference<GraphicsSimulation::Task> m_spawningStep;
		friend class ParticleTimestepKernel;
	};

	class JIMARA_API ParticleTimestepTask::Factory : public virtual Object {
	public:
		class Set;

		template<typename TaskType>
		inline static Reference<Factory> Of() {
			static const CreateFn createFn = [](GraphicsSimulation::Task* spawningStep) -> Reference<ParticleTimestepTask> {
				if (spawningStep == nullptr) return nullptr;
				const Reference<TaskType> task = Object::Instantiate<TaskType>(spawningStep->Context());
				task->m_spawningStep = spawningStep;
				return Reference<ParticleTimestepTask>(task.operator->());
			};
			const Reference<Factory> instance = new Factory(createFn, TypeId::Of<TaskType>());
			instance->ReleaseRef();
			return instance;
		}

		inline virtual ~Factory() {}

		inline Reference<ParticleTimestepTask> CreateTask(GraphicsSimulation::Task* spawningStep) const { return m_createFn(spawningStep); }

		inline TypeId TaskType()const { return m_taskType; }

	private:
		typedef Reference<ParticleTimestepTask>(*CreateFn)(GraphicsSimulation::Task*);
		const CreateFn m_createFn;
		const TypeId m_taskType;

		inline Factory(const CreateFn& createFn, const TypeId& taskType) : m_createFn(createFn), m_taskType(taskType) {}
	};

	class JIMARA_API ParticleTimestepTask::Factory::Set : public virtual Object {
	public:
		static Reference<Set> Current();

		inline virtual ~Set() {}

		size_t Size()const;

		const Factory* operator[](size_t index)const;

		const Factory* operator[](const std::type_index& taskType)const;

		const Serialization::SerializerList::From<TypeId>* TypeSerializer()const;

	private:
		const Reference<const Object> m_data;

		inline Set(const Object* data) : m_data(data) {}
	};
}
