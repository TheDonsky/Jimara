#pragma once
#include "ParticleBuffers.h"
#include "ParticleSystemInfo.h"
#include "../../GraphicsSimulation/GraphicsSimulation.h"
#include "../../../Core/TypeRegistration/ObjectFactory.h"
#include "../../../Data/Serialization/Attributes/CustomEditorNameAttribute.h"
#include "../../../Data/Serialization/Attributes/InlineSerializerListAttribute.h"


namespace Jimara {
	template<typename ParticleTaskType>
	class JIMARA_API ParticleTaskSet {
	public:
		typedef ObjectFactory<ParticleTaskType, const ParticleSystemInfo*> TaskFactory;

		inline static constexpr size_t InvalidTaskIndex() { return ~size_t(0u); }

		struct JIMARA_API TaskId;
		class JIMARA_API TaskSetEntry;
		class JIMARA_API TaskLayer;
		class JIMARA_API Serializer;
		
		inline ParticleTaskSet(const ParticleSystemInfo* systemInfo, GraphicsSimulation::Task* dependency);

		inline virtual ~ParticleTaskSet();

		inline size_t LayerCount()const;

		inline void SetLayerCount(size_t layerCount);

		inline TaskLayer Layer(size_t index);

		inline void RemoveLayer(size_t index);

		inline void GetDependencies(const Callback<GraphicsSimulation::Task*>& recordDependency)const;

		inline void SetBuffers(ParticleBuffers* buffers);

	private:
		const Reference<const ParticleSystemInfo> m_systemInfo;
		const Reference<GraphicsSimulation::Task> m_dependency;
		Reference<ParticleBuffers> m_particleBuffers;
		
		struct TaskInfo {
			Reference<const TaskFactory> factory;
			Reference<ParticleTaskType> task;
		};
		
		typedef Stacktor<TaskInfo, 4u> LayerTasks;
		Stacktor<LayerTasks, 1u> m_layers;

		inline ParticleTaskSet(const ParticleTaskSet&) = delete;
		inline ParticleTaskSet(ParticleTaskSet&&) = delete;
		inline ParticleTaskSet& operator=(const ParticleTaskSet&) = delete;
		inline ParticleTaskSet& operator=(ParticleTaskSet&&) = delete;
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

		inline void SwapTaskIndex(size_t a, size_t b);

		inline void Clear();

		inline void GetDependencies(const Callback<GraphicsSimulation::Task*>& recordDependency)const;

	private:
		ParticleTaskSet* m_set = nullptr;
		size_t m_layerIndex = InvalidTaskIndex();

		inline static void GetDependencies(
			const ParticleTaskSet* set, size_t layerIndex,
			const Callback<GraphicsSimulation::Task*>& recordDependency);

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
	class JIMARA_API ParticleTaskSet<ParticleTaskType>::Serializer : public virtual Serialization::SerializerList::From<ParticleTaskSet<ParticleTaskType>> {
	public:
		inline Serializer(const std::string_view& name, const std::string_view& hint, const std::vector<Reference<const Object>>& attributes = {})
			: Serialization::ItemSerializer(name, hint, attributes) {}

		/// <summary>
		/// Gives access to sub-serializers/fields
		/// </summary>
		/// <param name="recordElement"> Each sub-serializer will be reported by invoking this callback with serializer & corresonding target as parameters </param>
		/// <param name="target"> Serializer target object </param>
		inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, ParticleTaskSet<ParticleTaskType>* target)const override;
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
		task->SetBuffers(m_set->m_particleBuffers);
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
	inline void ParticleTaskSet<ParticleTaskType>::TaskLayer::SwapTaskIndex(size_t a, size_t b) {
		LayerTasks& tasks = Tasks();
		if (tasks.Size() <= 0u) return;
		a = Math::Min(a, tasks.Size() - 1u);
		b = Math::Min(b, tasks.Size() - 1u);
		if (a != b)
			std::swap(tasks[a], tasks[b]);
	}

	template<typename ParticleTaskType>
	inline void ParticleTaskSet<ParticleTaskType>::TaskLayer::Clear() {
		LayerTasks& tasks = Tasks();
		while (tasks.Size() > 0u) 
			RemoveTask(tasks.Size() - 1u);
	}

	template<typename ParticleTaskType>
	inline void ParticleTaskSet<ParticleTaskType>::TaskLayer::GetDependencies(const Callback<GraphicsSimulation::Task*>& recordDependency)const {
		GetDependencies(m_set, m_layerIndex, recordDependency);
	}

	template<typename ParticleTaskType>
	inline void ParticleTaskSet<ParticleTaskType>::TaskLayer::GetDependencies(
		const ParticleTaskSet* set, size_t layerIndex,
		const Callback<GraphicsSimulation::Task*>& recordDependency) {
		if (set == nullptr) return;
		layerIndex--;
		if (layerIndex >= 0u && layerIndex < set->m_layers.Size())
			while (true) {
				const LayerTasks& tasks = set->m_layers[layerIndex];
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
		if (set->m_dependency != nullptr)
			recordDependency(set->m_dependency);
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

	template<typename ParticleTaskType>
	inline void ParticleTaskSet<ParticleTaskType>::GetDependencies(const Callback<GraphicsSimulation::Task*>& recordDependency)const {
		TaskLayer::GetDependencies(this, m_layers.Size(), recordDependency);
	}

	template<typename ParticleTaskType>
	inline void ParticleTaskSet<ParticleTaskType>::SetBuffers(ParticleBuffers* buffers) {
		m_particleBuffers = buffers;
		for (size_t i = 0u; i < m_layers.Size(); i++) {
			const LayerTasks& tasks = m_layers[i];
			const TaskInfo* ptr = tasks.Data();
			const TaskInfo* const end = ptr + tasks.Size();
			while (ptr < end) {
				ptr->task->SetBuffers(buffers);
				ptr++;
			}
		}
	}



	template<typename ParticleTaskType>
	inline void ParticleTaskSet<ParticleTaskType>::Serializer::GetFields(
		const Callback<Serialization::SerializedObject>& recordElement, ParticleTaskSet<ParticleTaskType>* target)const {
		if (target == nullptr) return;
		
		// Update layer count:
		{
			size_t initialLayerCount = target->LayerCount();
			static const auto serializer = Serialization::ValueSerializer<size_t>::Create("Layer Count", "Number of layers");
			recordElement(serializer->Serialize(initialLayerCount));
			target->SetLayerCount(initialLayerCount);
		}

		// Serializer per task:
		struct TaskDesc {
			Reference<const typename TaskFactory::Set> factories;
			TaskLayer layer;
			size_t* taskIndex;
		};
		struct TaskSerializer : public virtual Serialization::SerializerList::From<TaskDesc> {
			inline TaskSerializer(const std::string_view& hint, const std::vector<Reference<const Object>>& attributes = {})
				: Serialization::ItemSerializer("Task", hint, attributes) {}

			inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordTaskElement, TaskDesc* desc)const final override {
				TaskId taskId = (*desc->taskIndex) < desc->layer.TaskCount() ? desc->layer.Task(*desc->taskIndex) : TaskId();
				Reference<const TaskFactory> factory = taskId.factory;
				{
					static const typename TaskFactory::RegisteredInstanceSerializer serializer("Type", "Task Type");
					static const typename TaskFactory::RegisteredInstanceSerializer nullSerializer("Type", "Task Type",
						std::vector<Reference<const Object>>{ Object::Instantiate<Serialization::CustomEditorNameAttribute>("Add") });
					((factory != nullptr) ? serializer : nullSerializer).GetFields(recordTaskElement, &factory);
				}
				if (factory != taskId.factory) {
					if (factory != nullptr) {
						TaskId existingTask = desc->layer.FindTask(factory);
						if (existingTask == InvalidTaskIndex()) {
							if (taskId.factory != nullptr) {
								taskId = desc->layer.GetTask(factory);
								if (taskId.task != nullptr) {
									desc->layer.RemoveTask(*desc->taskIndex);
									for (size_t i = desc->layer.TaskCount() - 1u; i > (*desc->taskIndex); i--)
										desc->layer.SwapTaskIndex(i, i - 1u);
								}
								else desc->layer.m_set->m_systemInfo->Context()->Log()->Error(
									"ParticleTaskSet<", TypeId::Of<ParticleTaskType>().Name(), ">::Serializer::GetFields - ",
									"Failed to create task for '", factory->ItemName(), "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");
							}
							else taskId = desc->layer.GetTask(factory);
						}
						else desc->layer.m_set->m_systemInfo->Context()->Log()->Warning(
							"ParticleTaskSet<", TypeId::Of<ParticleTaskType>().Name(), ">::Serializer::GetFields - ",
							"Layer already contains task for '", factory->ItemName(), "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					}
					else if (taskId.factory != nullptr) {
						desc->layer.RemoveTask(*desc->taskIndex);
						(*desc->taskIndex)--;
					}
				}
				if (taskId.task != nullptr) 
					taskId.task->GetFields(recordTaskElement);
			}
		};

		// Make sure we have unique names for serializers per factory:
		const std::shared_ptr<std::unordered_map<Reference<const TaskFactory>, Reference<const TaskSerializer>>> taskSerializers = []() {
			static std::shared_ptr<std::unordered_map<Reference<const TaskFactory>, Reference<const TaskSerializer>>> lastSerializers;
			static SpinLock instanceLock;
			static void(*onRegistryChanged)() = nullptr; 

			std::unique_lock<SpinLock> lock(instanceLock);
			if (lastSerializers != nullptr) 
				return lastSerializers;

			const Reference<const typename TaskFactory::Set> factories = TaskFactory::All();

			if (onRegistryChanged == nullptr) {
				onRegistryChanged = []() {
					std::unique_lock<SpinLock> lock(instanceLock);
					lastSerializers = nullptr;
					TypeId::OnRegisteredTypeSetChanged() -= Callback<>(onRegistryChanged);
					onRegistryChanged = nullptr;
				};
				TypeId::OnRegisteredTypeSetChanged() += Callback<>(onRegistryChanged);
			}

			if (lastSerializers == nullptr) {
				lastSerializers = std::make_shared<std::unordered_map<Reference<const TaskFactory>, Reference<const TaskSerializer>>>();
				for (size_t i = 0; i < factories->Size(); i++) {
					const TaskFactory* factory = factories->At(i);
					lastSerializers->insert(std::make_pair<Reference<const TaskFactory>, Reference<const TaskSerializer>>(
						factory, Object::Instantiate<TaskSerializer>(factory->Hint(),
							std::vector<Reference<const Object>>{ Object::Instantiate<Serialization::CustomEditorNameAttribute>(factory->ItemName()) })));
				}
			}

			return lastSerializers;
		}();

		// Serializer for individual layers:
		struct LayerInfo {
			const std::unordered_map<Reference<const TaskFactory>, Reference<const TaskSerializer>>* taskSerializers;
			Reference<const typename TaskFactory::Set> factories;
			TaskLayer layer;
		};
		struct LayerSerializer : public virtual Serialization::SerializerList::From<LayerInfo> {
			inline LayerSerializer(const std::string_view& name, const std::string_view& hint, const std::vector<Reference<const Object>>& attributes = {})
				: Serialization::ItemSerializer(name, hint, attributes) {}

			inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordLayerElement, LayerInfo* layerInfo)const final override {
				for (size_t i = 0u; i <= layerInfo->layer.TaskCount(); i++) {
					Reference<const TaskSerializer> serializer;
					if (i < layerInfo->layer.TaskCount()) {
						TaskId task = layerInfo->layer.Task(i);
						const auto it = layerInfo->taskSerializers->find(task.factory);
						if (it != layerInfo->taskSerializers->end())
							serializer = it->second;
						else serializer = Object::Instantiate<TaskSerializer>(task.factory->Hint(),
							std::vector<Reference<const Object>>{ Object::Instantiate<Serialization::CustomEditorNameAttribute>(task.factory->ItemName()) });
					}
					else {
						static const TaskSerializer addSerializer("Add task to the layer",
							std::vector<Reference<const Object>>{ Serialization::InlineSerializerListAttribute::Instance() });
						serializer = &addSerializer;
					}

					TaskDesc desc;
					desc.factories = layerInfo->factories;
					desc.layer = layerInfo->layer;
					desc.taskIndex = &i;
					const size_t startI = i;
					recordLayerElement(serializer->Serialize(desc));
				}
			}
		};

		// Make sure different layer serializers have different names:
		const std::shared_ptr<std::vector<Reference<const LayerSerializer>>> layerSerializers = [&]() {
			static const constexpr std::string_view serializerHint =
				"Simulation is arranged in a sequence of layers; each layer runs right after the previous one, \n"
				"while the order of execution for individual tasks within the same layer is largely undefined.";
			if (target->LayerCount() <= 1u) {
				static const std::shared_ptr<std::vector<Reference<const LayerSerializer>>> firstLayerSerializer =
					std::make_shared<std::vector<Reference<const LayerSerializer>>>(std::vector<Reference<const LayerSerializer>> {
						Object::Instantiate<LayerSerializer>("Layer 0", serializerHint,
						std::vector<Reference<const Object>>{ Serialization::InlineSerializerListAttribute::Instance() }) });
				return firstLayerSerializer;
			}
			static std::shared_ptr<std::vector<Reference<const LayerSerializer>>> lastSerializers;
			static SpinLock serializerLock;
			std::unique_lock<SpinLock> lock(serializerLock);
			if (lastSerializers == nullptr || lastSerializers->size() < target->LayerCount()) {
				lastSerializers = std::make_shared<std::vector<Reference<const LayerSerializer>>>();
				for (size_t i = 0u; i < target->LayerCount(); i++) {
					std::stringstream stream; stream << "Layer " << i; const std::string name = stream.str();
					lastSerializers->push_back(Object::Instantiate<LayerSerializer>(name, serializerHint));
				}
			}
			return lastSerializers;
		}();

		// Serialize layers:
		LayerInfo layerInfo;
		layerInfo.taskSerializers = taskSerializers.operator->();
		layerInfo.factories = TaskFactory::All();
		for (size_t i = 0u; i < target->LayerCount(); i++) {
			layerInfo.layer = target->Layer(i);
			recordElement(layerSerializers->operator[](i)->Serialize(layerInfo));
		}
	}
}
