#include "ParticleKernels.h"
#include "../../../Data/Serialization/Attributes/EnumAttribute.h"



namespace Jimara {
	namespace {
		template<typename TaskType>
		struct ParticleKernel_FactorySetData : public virtual Object {
			typedef typename TaskType::Factory Factory;
			typedef typename TaskType::Factory::Set FactorySet;

			class TypeIdSerializer : public virtual Serialization::SerializerList::From<TypeId> {
			private:
				const Reference<const Serialization::ItemSerializer::Of<std::string_view>> m_typeNameSerializer;
				const std::unordered_map<std::string_view, TypeId> m_typeIdMappings;

			public:
				inline TypeIdSerializer(const std::vector<Reference<const Factory>>& factories)
					: Serialization::ItemSerializer("Type", "Particle task type")
					, m_typeNameSerializer(Serialization::StringViewSerializer::Create("Type", "Particle task type", std::vector<Reference<const Object>>({
						[&]()->Reference<const Object> {
							std::vector<Serialization::EnumAttribute<std::string_view>::Choice> choices;
							for (size_t i = 0u; i < factories.size(); i++)
								choices.push_back(Serialization::EnumAttribute<std::string_view>::Choice(factories[i]->TaskType().Name(), factories[i]->TaskType().Name()));
							return Object::Instantiate<Serialization::EnumAttribute<std::string_view>>(choices, false);
						}()
						})))
					, m_typeIdMappings([&]() -> std::unordered_map<std::string_view, TypeId> {
							std::unordered_map<std::string_view, TypeId> mappings;
							for (size_t i = 0u; i < factories.size(); i++)
								mappings[factories[i]->TaskType().Name()] = factories[i]->TaskType();
							return mappings;
						}()) {
				}

				inline virtual ~TypeIdSerializer() {}

				inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, TypeId* target)const final override {
					std::string_view typeName = target->Name();
					recordElement(m_typeNameSerializer->Serialize(typeName));
					if (typeName != target->Name()) return;
					const auto it = m_typeIdMappings.find(typeName);
					if (it == m_typeIdMappings.end()) (*target) = TypeId();
					else (*target) = it->second;
				}
			};

			std::vector<Reference<const Factory>> factories;
			std::unordered_map<std::type_index, size_t> indexByType;
			Reference<const TypeIdSerializer> typeIdSerializer;

			template<typename SetCreateFn>
			inline static Reference<FactorySet> All(const SetCreateFn& createSet) {
				static std::mutex instanceLock;
				static Reference<FactorySet> instance;
				static bool subscribedToTypeRegistry = false;

				// If old instance is still valid, we return it:
				Reference<FactorySet> all = instance;
				if (all != nullptr) return all;

				// Create new set:
				const Reference<ParticleKernel_FactorySetData> set = Object::Instantiate<ParticleKernel_FactorySetData>();
				all = instance = createSet(set);

				// Find all factories:
				{
					const Reference<RegisteredTypeSet> currentTypes = RegisteredTypeSet::Current();
					std::unordered_set<TypeId> registeredTypes;
					for (size_t i = 0; i < currentTypes->Size(); i++) {
						auto checkAttribute = [&](const Object* attribute) {
							const Factory* instanceAttribute = dynamic_cast<const Factory*>(attribute);
							if (instanceAttribute == nullptr) return;
							if (registeredTypes.find(instanceAttribute->TaskType()) != registeredTypes.end()) return;
							registeredTypes.insert(instanceAttribute->TaskType());
							set->factories.push_back(instanceAttribute);
						};
						currentTypes->At(i).GetAttributes(Callback<const Object*>::FromCall(&checkAttribute));
					}
				}

				// Sort attributes by name:
				std::sort(set->factories.begin(), set->factories.end(),
					[](const Reference<const Factory>& a, const Reference<const Factory>& b) {
						return a->TaskType().Name() < b->TaskType().Name();
					});

				// Fill in index:
				for (size_t i = 0; i < set->factories.size(); i++)
					set->indexByType[set->factories[i]->TaskType().TypeIndex()] = i;

				// Create serializer:
				set->typeIdSerializer = Object::Instantiate<TypeIdSerializer>(set->factories);

				// Make sure we are subscribed to the OnRegisteredTypeSetChanged event:
				if (!subscribedToTypeRegistry) {
					static auto onRegisteredTypeSetChanged = []() {
						std::unique_lock<std::mutex> lock(instanceLock);
						instance = nullptr;
					};
					TypeId::OnRegisteredTypeSetChanged() += Callback<>::FromCall(&onRegisteredTypeSetChanged);
					subscribedToTypeRegistry = true;
				}

				// Done:
				return all;
			}
		};
	}

	void ParticleInitializationTask::SetBuffers(ParticleBuffers* buffers) {
		std::unique_lock<SpinLock> lock(m_dependencyLock);
		m_dependencies.Clear();
		auto findBuffer = [&](const ParticleBuffers::BufferId* bufferId) -> Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* {
			if (buffers == nullptr) return nullptr;
			ParticleBuffers::BufferInfo bufferInfo = buffers->GetBufferInfo(bufferId);
			if (bufferInfo.allocationTask != nullptr) 
				m_dependencies.Push(bufferInfo.allocationTask);
			return bufferInfo.buffer;
		};
		m_buffers = buffers;
		SetBuffers(
			findBuffer(ParticleBuffers::IndirectionBufferId()),
			findBuffer(ParticleBuffers::LiveParticleCountBufferId()),
			ParticleBuffers::BufferSearchFn::FromCall(&findBuffer));
	}

	void ParticleInitializationTask::Synchronize() {
		m_lastBuffers = m_buffers;
		UpdateSettings();
	}

	void ParticleInitializationTask::GetDependencies(const Callback<GraphicsSimulation::Task*>& recordDependency)const {
		std::unique_lock<SpinLock> lock(m_dependencyLock);
		const Reference<GraphicsSimulation::Task>* taskPtr = m_dependencies.Data();
		const Reference<GraphicsSimulation::Task>* const end = taskPtr + m_dependencies.Size();
		while (taskPtr < end) {
			recordDependency(taskPtr->operator->());
			taskPtr++;
		}
	}

	Reference<ParticleInitializationTask::Factory::Set> ParticleInitializationTask::Factory::Set::Current() {
		auto createInstance = [](const Object* data) -> Reference<Set> {
			const Reference<ParticleInitializationTask::Factory::Set> instance = new ParticleInitializationTask::Factory::Set(data);
			instance->ReleaseRef();
			return instance;
		};
		return ParticleKernel_FactorySetData<ParticleInitializationTask>::All(createInstance);
	}

	size_t ParticleInitializationTask::Factory::Set::Size()const {
		return dynamic_cast<const ParticleKernel_FactorySetData<ParticleInitializationTask>*>(m_data.operator->())->factories.size();
	}

	const ParticleInitializationTask::Factory* ParticleInitializationTask::Factory::Set::operator[](size_t index)const {
		return dynamic_cast<const ParticleKernel_FactorySetData<ParticleInitializationTask>*>(m_data.operator->())->factories[index];
	}

	const ParticleInitializationTask::Factory* ParticleInitializationTask::Factory::Set::operator[](const std::type_index& taskType)const {
		const auto data = dynamic_cast<const ParticleKernel_FactorySetData<ParticleInitializationTask>*>(m_data.operator->());
		const auto it = data->indexByType.find(taskType);
		return (it == data->indexByType.end()) ? nullptr : data->factories[it->second];
	}

	const Serialization::SerializerList::From<TypeId>* ParticleInitializationTask::Factory::Set::TypeSerializer()const {
		return dynamic_cast<const ParticleKernel_FactorySetData<ParticleInitializationTask>*>(m_data.operator->())->typeIdSerializer;
	}





	void ParticleTimestepTask::SetBuffers(ParticleBuffers* buffers) {
		m_buffers = buffers;
		auto findNull = [](const ParticleBuffers::BufferId*) -> Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* { return nullptr; };
		SetBuffers(buffers == nullptr
			? ParticleBuffers::BufferSearchFn::FromCall(&findNull)
			: ParticleBuffers::BufferSearchFn(&ParticleBuffers::GetBuffer, buffers));
	}

	void ParticleTimestepTask::Synchronize() {
		m_lastBuffers = m_buffers;
		UpdateSettings();
	}

	void ParticleTimestepTask::GetDependencies(const Callback<GraphicsSimulation::Task*>& recordDependency)const {
		recordDependency(m_spawningStep);
	}

	Reference<ParticleTimestepTask::Factory::Set> ParticleTimestepTask::Factory::Set::Current() {
		auto createInstance = [](const Object* data) -> Reference<Set> {
			const Reference<ParticleTimestepTask::Factory::Set> instance = new ParticleTimestepTask::Factory::Set(data);
			instance->ReleaseRef();
			return instance;
		};
		return ParticleKernel_FactorySetData<ParticleTimestepTask>::All(createInstance);
	}

	size_t ParticleTimestepTask::Factory::Set::Size()const {
		return dynamic_cast<const ParticleKernel_FactorySetData<ParticleTimestepTask>*>(m_data.operator->())->factories.size();
	}

	const ParticleTimestepTask::Factory* ParticleTimestepTask::Factory::Set::operator[](size_t index)const {
		return dynamic_cast<const ParticleKernel_FactorySetData<ParticleTimestepTask>*>(m_data.operator->())->factories[index];
	}

	const ParticleTimestepTask::Factory* ParticleTimestepTask::Factory::Set::operator[](const std::type_index& taskType)const {
		const auto data = dynamic_cast<const ParticleKernel_FactorySetData<ParticleTimestepTask>*>(m_data.operator->());
		const auto it = data->indexByType.find(taskType);
		return (it == data->indexByType.end()) ? nullptr : data->factories[it->second];
	}

	const Serialization::SerializerList::From<TypeId>* ParticleTimestepTask::Factory::Set::TypeSerializer()const {
		return dynamic_cast<const ParticleKernel_FactorySetData<ParticleTimestepTask>*>(m_data.operator->())->typeIdSerializer;
	}
}
