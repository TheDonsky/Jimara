#include "ParticleKernels.h"



namespace Jimara {
	namespace {
		template<typename KernelType>
		struct ParticleKernel_InstanceAttribute : public virtual Object {
			const Reference<const KernelType> kernelInstance;
			const TypeId typeId;

			inline ParticleKernel_InstanceAttribute(const KernelType* kernel, TypeId type)
				: kernelInstance(kernel), typeId(type) {}

			struct Set : public virtual Object {
				std::vector<Reference<const ParticleKernel_InstanceAttribute>> attributes;
				std::unordered_map<const KernelType*, size_t> indexByKernel;
				std::unordered_map<TypeId, size_t> indexByType;
			};

			template<typename SetType, typename SetCreateFn>
			inline static Reference<SetType> All(const SetCreateFn& createSet) {
				static std::mutex instanceLock;
				static Reference<SetType> instance;
				static bool subscribedToTypeRegistry = false;

				static auto onRegisteredTypeSetChanged = []() {
					std::unique_lock<std::mutex> lock(instanceLock);
					instance = nullptr;
				};

				std::unique_lock<std::mutex> lock(instanceLock);

				// If old instance is still valid, we return it:
				Reference<SetType> all = instance;
				if (all != nullptr) return all;

				// Create new set:
				const Reference<Set> set = Object::Instantiate<Set>();
				all = instance = createSet(set);
				
				// Find all attributes:
				{
					const Reference<RegisteredTypeSet> currentTypes = RegisteredTypeSet::Current();
					std::unordered_set<TypeId> registeredTypes;
					for (size_t i = 0; i < currentTypes->Size(); i++) {
						auto checkAttribute = [&](const Object* attribute) {
							const ParticleKernel_InstanceAttribute* instanceAttribute = dynamic_cast<const ParticleKernel_InstanceAttribute*>(attribute);
							if (instanceAttribute == nullptr || instanceAttribute->kernelInstance == nullptr) return;
							if (registeredTypes.find(instanceAttribute->typeId) != registeredTypes.end()) return;
							registeredTypes.insert(instanceAttribute->typeId);
							set->attributes.push_back(instanceAttribute);
						};
						currentTypes->At(i).GetAttributes(Callback<const Object*>::FromCall(&checkAttribute));
					}
				}
				
				// Sort attributes by name:
				std::sort(set->attributes.begin(), set->attributes.end(),
					[](const Reference<const ParticleKernel_InstanceAttribute>& a, const Reference<const ParticleKernel_InstanceAttribute>& b) {
						return a->typeId.Name() < b->typeId.Name();
					});

				// Fill in indexes:
				for (size_t i = 0; i < set->attributes.size(); i++) {
					const ParticleKernel_InstanceAttribute* attribute = set->attributes[i];
					set->indexByKernel[attribute->kernelInstance] = i;
					set->indexByType[attribute->typeId] = i;
				}

				// Make sure we are subscribed to the OnRegisteredTypeSetChanged event:
				if (!subscribedToTypeRegistry) {
					TypeId::OnRegisteredTypeSetChanged() += Callback<>::FromCall(&onRegisteredTypeSetChanged);
					subscribedToTypeRegistry = true;
				}

				// Done:
				return all;
			}
		};
	}

	void ParticleInitializationKernel::Task::SetBuffers(ParticleBuffers* buffers) {
		std::unique_lock<SpinLock> lock(m_dependencyLock);
		m_dependencies.Clear();
		auto findBuffer = [&](const ParticleBuffers::BufferId* bufferId) -> Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* {
			if (buffers == nullptr) return nullptr;
			ParticleBuffers::BufferInfo bufferInfo = buffers->GetBufferInfo(bufferId);
			if (bufferInfo.allocationTask != nullptr) 
				m_dependencies.Push(bufferInfo.allocationTask);
			return bufferInfo.buffer;
		};
		SetBuffers(
			findBuffer(ParticleBuffers::IndirectionBufferId()),
			findBuffer(ParticleBuffers::LiveParticleCountBufferId()),
			BufferSearchFn::FromCall(&findBuffer));
	}

	void ParticleInitializationKernel::Task::GetDependencies(const Callback<GraphicsSimulation::Task*>& recordDependency)const {
		std::unique_lock<SpinLock> lock(m_dependencyLock);
		const Reference<GraphicsSimulation::Task>* taskPtr = m_dependencies.Data();
		const Reference<GraphicsSimulation::Task>* const end = taskPtr + m_dependencies.Size();
		while (taskPtr < end) {
			recordDependency(taskPtr->operator->());
			taskPtr++;
		}
	}

	size_t ParticleInitializationKernel::Set::Size()const {
		return dynamic_cast<const ParticleKernel_InstanceAttribute<ParticleInitializationKernel>::Set*>(m_data.operator->())->attributes.size();
	}

	std::pair<const ParticleInitializationKernel*, TypeId> ParticleInitializationKernel::Set::operator[](size_t index)const {
		const ParticleKernel_InstanceAttribute<ParticleInitializationKernel>* entry =
			dynamic_cast<const ParticleKernel_InstanceAttribute<ParticleInitializationKernel>::Set*>(m_data.operator->())->attributes[index];
		return std::make_pair(entry->kernelInstance.operator->(), entry->typeId);
	}

	size_t ParticleInitializationKernel::Set::operator[](const ParticleInitializationKernel* kernel)const {
		const auto data = dynamic_cast<const ParticleKernel_InstanceAttribute<ParticleInitializationKernel>::Set*>(m_data.operator->());
		const auto it = data->indexByKernel.find(kernel);
		return (it == data->indexByKernel.end()) ? InvalidIndex() : it->second;
	}

	size_t ParticleInitializationKernel::Set::operator[](const TypeId& type)const {
		const auto data = dynamic_cast<const ParticleKernel_InstanceAttribute<ParticleInitializationKernel>::Set*>(m_data.operator->());
		const auto it = data->indexByType.find(type);
		return (it == data->indexByType.end()) ? InvalidIndex() : it->second;
	}

	Reference<ParticleInitializationKernel::Set> ParticleInitializationKernel::All() {
		auto createInstance = [](const Object* data) -> Reference<Set> {
			const Reference<ParticleInitializationKernel::Set> instance = new ParticleInitializationKernel::Set(data);
			instance->ReleaseRef();
			return instance;
		};
		return ParticleKernel_InstanceAttribute<ParticleInitializationKernel>::All<Set>(createInstance);
	}

	Reference<Object> ParticleInitializationKernel::CreateInstanceAttribute(const ParticleInitializationKernel* instance, TypeId type) {
		return Object::Instantiate<ParticleKernel_InstanceAttribute<ParticleInitializationKernel>>(instance, type);
	}

	Reference<ParticleTimestepKernel::Task> ParticleTimestepKernel::CreateTask(GraphicsSimulation::Task* spawningStep) {
		if (spawningStep == nullptr) return nullptr;
		Reference<ParticleTimestepKernel::Task> task = CreateTask(spawningStep->Context());
		if (task == nullptr) {
			spawningStep->Context()->Log()->Error("ParticleTimestepKernel::CreateTask - CreateTask(Context) failed! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}
		if (task->m_spawningStep != nullptr) {
			spawningStep->Context()->Log()->Error("ParticleTimestepKernel::CreateTask - CreateTask(Context) returned an existing instance! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}
		task->m_spawningStep = spawningStep;
		return task;
	}

	void ParticleTimestepKernel::Task::GetDependencies(const Callback<GraphicsSimulation::Task*>& recordDependency)const {
		recordDependency(m_spawningStep);
	}

	size_t ParticleTimestepKernel::Set::Size()const {
		return dynamic_cast<const ParticleKernel_InstanceAttribute<ParticleTimestepKernel>::Set*>(m_data.operator->())->attributes.size();
	}

	std::pair<const ParticleTimestepKernel*, TypeId> ParticleTimestepKernel::Set::operator[](size_t index)const {
		const ParticleKernel_InstanceAttribute<ParticleTimestepKernel>* entry =
			dynamic_cast<const ParticleKernel_InstanceAttribute<ParticleTimestepKernel>::Set*>(m_data.operator->())->attributes[index];
		return std::make_pair(entry->kernelInstance.operator->(), entry->typeId);
	}

	size_t ParticleTimestepKernel::Set::operator[](const ParticleTimestepKernel* kernel)const {
		const auto data = dynamic_cast<const ParticleKernel_InstanceAttribute<ParticleTimestepKernel>::Set*>(m_data.operator->());
		const auto it = data->indexByKernel.find(kernel);
		return (it == data->indexByKernel.end()) ? InvalidIndex() : it->second;
	}

	size_t ParticleTimestepKernel::Set::operator[](const TypeId& type)const {
		const auto data = dynamic_cast<const ParticleKernel_InstanceAttribute<ParticleTimestepKernel>::Set*>(m_data.operator->());
		const auto it = data->indexByType.find(type);
		return (it == data->indexByType.end()) ? InvalidIndex() : it->second;
	}

	Reference<ParticleTimestepKernel::Set> ParticleTimestepKernel::All() {
		auto createInstance = [](const Object* data) -> Reference<Set> {
			const Reference<ParticleTimestepKernel::Set> instance = new ParticleTimestepKernel::Set(data);
			instance->ReleaseRef();
			return instance;
		};
		return ParticleKernel_InstanceAttribute<ParticleTimestepKernel>::All<Set>(createInstance);
	}

	Reference<Object> ParticleTimestepKernel::CreateInstanceAttribute(const ParticleTimestepKernel* instance, TypeId type) {
		return Object::Instantiate<ParticleKernel_InstanceAttribute<ParticleTimestepKernel>>(instance, type);
	}
}
