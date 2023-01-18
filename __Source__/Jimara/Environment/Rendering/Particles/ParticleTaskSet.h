#pragma once
#include "ParticleSystemInfo.h"
#include "../../GraphicsSimulation/GraphicsSimulation.h"
#include "../../../Core/TypeRegistration/ObjectFactory.h"


namespace Jimara {
	template<typename ParticleTaskType>
	class JIMARA_API ParticleTaskSet {
	public:
		typedef ObjectFactory<ParticleTaskType, const ParticleSystemInfo*> TaskFactory;

		inline static constexpr size_t InvalidTaskIndex() { return ~size_t(0u); }

		struct JIMARA_API TaskId;
		class JIMARA_API TaskSetEntry;
		class JIMARA_API TaskLayer;
		
		inline ParticleTaskSet(const ParticleSystemInfo* systemInfo, GraphicsSimulation::Task* dependency);

		inline virtual ~ParticleTaskSet();

		inline size_t LayerCount()const;

		inline void SetLayerCount(size_t layerCount);

		inline TaskLayer Layer(size_t index);

		inline void RemoveLayer(size_t index);

	private:
		const Reference<const ParticleSystemInfo> m_systemInfo;
		const Reference<GraphicsSimulation::Task> m_dependency;
		
		struct TaskInfo {
			Reference<const TaskFactory> factory;
			Reference<ParticleTaskType> task;
		};
		
		typedef Stacktor<TaskInfo, 4u> LayerTasks;
		Stacktor<LayerTasks, 1u> m_layers;
	};



	template<typename ParticleTaskType>
	struct JIMARA_API ParticleTaskSet<ParticleTaskType>::TaskId {
		size_t index = InvalidTaskIndex();
		ParticleTaskType* task = nullptr;
		const TaskFactory* factory = nullptr;

		inline operator size_t()const { return index; }
		inline operator ParticleTaskType* ()const { return task; }
		inline operator const TaskFactory* ()const { return factory; }
	};



	template<typename ParticleTaskType>
	class JIMARA_API ParticleTaskSet<ParticleTaskType>::TaskLayer {
	public:
		typedef typename ParticleTaskSet<ParticleTaskType>::TaskId TaskId;

		inline TaskLayer() {};

		inline size_t TaskCount()const;

		inline TaskId Task(size_t index)const;

		inline TaskId FindTask(const TaskFactory* factory)const;

		inline TaskId GetTask(const TaskFactory* factory);

		inline TaskId TaskIndex(ParticleTaskType* factory)const;

		inline void RemoveTask(size_t index);

		inline void RemoveTask(const TaskFactory* factory);

		inline void RemoveTask(ParticleTaskType* task);

		inline void RemoveTask(const TaskId& index);

		inline void Clear();

		inline void GetDependencies(const Callback<GraphicsSimulation::Task*>& recordDependency)const;

	private:
		ParticleTaskSet* const m_set = nullptr;
		const size_t m_layerIndex = InvalidTaskIndex();

		friend class TaskSetEntry;
		friend class ParticleTaskSet<ParticleTaskType>;
		inline TaskLayer(ParticleTaskSet* set, size_t layerIndex) : m_set(set), m_layerIndex(layerIndex) {}
		inline LayerTasks& Tasks() { return m_set->m_layers[m_layerIndex]; }
		inline const LayerTasks& Tasks()const { return m_set->m_layers[m_layerIndex]; }
	};



	template<typename ParticleTaskType>
	class JIMARA_API ParticleTaskSet<ParticleTaskType>::TaskSetEntry {
	public:
		inline virtual ~TaskSetEntry() {}

		inline void GetParticleTaskSetDependencies(const Callback<GraphicsSimulation::Task*>& recordDependency)const {
			return TaskLayer(m_set, m_layerIndex).GetDependencies(recordDependency);
		}

	private:
		ParticleTaskSet* m_set = nullptr;
		size_t m_layerIndex = InvalidTaskIndex();
		friend class TaskLayer;
		friend class ParticleTaskSet<ParticleTaskType>;
		void ParticleTaskSetEntry_Configure(ParticleTaskSet* set, size_t layerIndex) { m_set = set; m_layerIndex = layerIndex; }
		void ParticleTaskSetEntry_Clear() { ParticleTaskSetEntry_Configure(nullptr, InvalidTaskIndex()); }
	};





	template<typename ParticleTaskType>
	inline size_t ParticleTaskSet<ParticleTaskType>::TaskLayer::TaskCount()const {
		return Tasks().Size();
	}

	template<typename ParticleTaskType>
	inline typename ParticleTaskSet<ParticleTaskType>::TaskId ParticleTaskSet<ParticleTaskType>::TaskLayer::Task(size_t index)const {
		const LayerTasks& tasks = Tasks();
		if (index >= tasks.Size()) return {};
		const TaskInfo& task = tasks[index];
		return { index, task.task.operator->(), task.factory.operator->() };
	}

	template<typename ParticleTaskType>
	inline typename ParticleTaskSet<ParticleTaskType>::TaskId ParticleTaskSet<ParticleTaskType>::TaskLayer::FindTask(const TaskFactory* factory)const {
		if (factory == nullptr) return {};
		const LayerTasks& tasks = Tasks();
		for (size_t i = 0; i < tasks.Size(); i++) {
			const TaskInfo& task = tasks[i];
			if (task.factory == factory)
				return { i, task.task.operator->(), factory };
		}
		return {};
	}

	template<typename ParticleTaskType>
	inline typename ParticleTaskSet<ParticleTaskType>::TaskId ParticleTaskSet<ParticleTaskType>::TaskLayer::GetTask(const TaskFactory* factory) {
		if (factory == nullptr) return {};
		{
			const TaskId id = FindTask(factory);
			if (id != InvalidTaskIndex()) return id;
		}
		const Reference<ParticleTaskType> task = factory->CreateInstance(m_set->m_systemInfo);
		if (task == nullptr) {
			m_set->m_systemInfo->Context()->Log()->Error(
				"ParticleTaskSet<", TypeId::Of<ParticleTaskType>().Name(), ">::TaskLayer::GetTask - Failed to create an instance! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return {};
		}
		task->ParticleTaskSetEntry_Configure(m_set, m_layerIndex);
		LayerTasks& tasks = Tasks();
		tasks.Push({ factory, task });
		return { tasks.Size() - 1u, task.operator->(), factory };
	}

	template<typename ParticleTaskType>
	inline typename ParticleTaskSet<ParticleTaskType>::TaskId ParticleTaskSet<ParticleTaskType>::TaskLayer::TaskIndex(ParticleTaskType* task)const {
		if (task == nullptr) return {};
		const LayerTasks& tasks = Tasks(this);
		for (size_t i = 0; i < tasks.Size(); i++) {
			const TaskInfo& info = tasks[i];
			if (info.task == task)
				return { i, task, info.factory.operator->() };
		}
		return {};
	}

	template<typename ParticleTaskType>
	inline void ParticleTaskSet<ParticleTaskType>::TaskLayer::RemoveTask(size_t index) {
		LayerTasks& tasks = Tasks();
		if (index >= tasks.Size()) return;
		tasks[index].task->ParticleTaskSetEntry_Clear();
		tasks.RemoveAt(index);
	}

	template<typename ParticleTaskType>
	inline void ParticleTaskSet<ParticleTaskType>::TaskLayer::RemoveTask(const TaskFactory* factory) {
		RemoveTask(FindTask(factory).index);
	}

	template<typename ParticleTaskType>
	inline void ParticleTaskSet<ParticleTaskType>::TaskLayer::RemoveTask(ParticleTaskType* task) {
		RemoveTask(FindTask(task).index);
	}

	template<typename ParticleTaskType>
	inline void ParticleTaskSet<ParticleTaskType>::TaskLayer::RemoveTask(const TaskId& index) {
		RemoveTask(index.index);
	}

	template<typename ParticleTaskType>
	inline void ParticleTaskSet<ParticleTaskType>::TaskLayer::Clear() {
		LayerTasks& tasks = Tasks();
		while (tasks.Size() > 0u) 
			RemoveTask(tasks.Size() - 1u);
	}

	template<typename ParticleTaskType>
	inline void ParticleTaskSet<ParticleTaskType>::TaskLayer::GetDependencies(const Callback<GraphicsSimulation::Task*>& recordDependency)const {
		if (m_set == nullptr) return;
		size_t layerIndex = (m_layerIndex - 1u);
		if (layerIndex < 0u || layerIndex >= m_set->m_layers.Size()) return;
		while (true) {
			const LayerTasks& tasks = m_set->m_layers[layerIndex];
			if (tasks.Size() > 0u) {
				const typename ParticleTaskSet<ParticleTaskType>::TaskInfo* ptr = tasks.Data();
				const typename ParticleTaskSet<ParticleTaskType>::TaskInfo* const end = ptr + tasks.Size();
				while (ptr < end) {
					recordDependency(ptr->task);
					ptr++;
				}
				return;
			}
			else if (layerIndex <= 0u) break;
			else layerIndex--;
		}
		if (m_set->m_dependency != nullptr)
			recordDependency(m_set->m_dependency);
	}





	template<typename ParticleTaskType>
	inline ParticleTaskSet<ParticleTaskType>::ParticleTaskSet(const ParticleSystemInfo* systemInfo, GraphicsSimulation::Task* dependency) 
		: m_systemInfo(systemInfo), m_dependency(dependency) {}

	template<typename ParticleTaskType>
	inline ParticleTaskSet<ParticleTaskType>::~ParticleTaskSet() {
		SetLayerCount(0u);
	}

	template<typename ParticleTaskType>
	inline size_t ParticleTaskSet<ParticleTaskType>::LayerCount()const {
		return m_layers.Size();
	}

	template<typename ParticleTaskType>
	inline void ParticleTaskSet<ParticleTaskType>::SetLayerCount(size_t layerCount) {
		while (m_layers.Size() > layerCount) {
			Layer(m_layers.Size() - 1u).Clear();
			m_layers.Pop();
		}
		m_layers.Resize(layerCount);
	}

	template<typename ParticleTaskType>
	inline typename ParticleTaskSet<ParticleTaskType>::TaskLayer ParticleTaskSet<ParticleTaskType>::Layer(size_t index) {
		assert(index < m_layers.Size());
		return TaskLayer(this, index);
	}

	template<typename ParticleTaskType>
	inline void ParticleTaskSet<ParticleTaskType>::RemoveLayer(size_t index) {
		if (index >= m_layers.Size()) return;
		Layer(index).Clear();
		m_layers.RemoveAt(index);
		while (index < m_layers.Size()) {
			const LayerTasks& tasks = m_layers[index];
			const TaskInfo* ptr = tasks.Data();
			const TaskInfo* const end = ptr + tasks.Size();
			while (ptr < end) {
				ptr->task->ParticleTaskSetEntry_Configure(this, index);
				ptr++;
			}
			index++;
		}
	}
}
