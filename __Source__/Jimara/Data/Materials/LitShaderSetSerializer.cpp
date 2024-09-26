#include "LitShaderSetSerializer.h"
#include "../Serialization/Helpers/SerializerMacros.h"


namespace Jimara {
	LitShaderSetSerializer::LitShaderSetSerializer(
		const std::string_view& name, const std::string_view& hint, const std::vector<Reference<const Object>>& attributes)
		: Serialization::ItemSerializer(name, hint, attributes) {}

	LitShaderSetSerializer::~LitShaderSetSerializer() {}

	void LitShaderSetSerializer::GetFields(
		const Callback<Serialization::SerializedObject>& recordElement, 
		Reference<const Material::LitShaderSet>* target)const {
		
		if (target == nullptr)
			return;

		struct State {
			size_t count = 0u;
			bool dirty = false;
		};

		State state = {};
		
#define JM_LitShaderSetSerializer_SERIALIZE_FIELD(field, name, hint, ...) \
		JIMARA_SERIALIZE_FIELDS(&state, recordElement) { \
			std::remove_reference_t<std::remove_pointer_t<decltype(&field)>> value = field; \
			JIMARA_SERIALIZE_FIELD(value, name, hint, __VA_ARGS__); \
			if (value != field) { \
				field = value; \
				state.dirty = true; \
			} \
		}

		// Count:
		{
			if ((*target) != nullptr)
				state.count = (*target)->Size();
			JM_LitShaderSetSerializer_SERIALIZE_FIELD(state.count, "Count", "Number of shaders");
		}

		// Entries:
		struct EntryState {
			Reference<const Material::LitShader> shader;
			std::string shaderPath;
			std::vector<Material::EditorPath> editorPaths;
			uint32_t blendMode = 0u;
			uint32_t materialFlags = 0u;
			std::vector<Material::Property> properties;
			size_t shadingStateSize = 1u;
			bool dirty = false;
		};
		struct EntrySerializer : public virtual Serialization::SerializerList::From<EntryState> {
			inline EntrySerializer(const std::string_view& name, const std::string_view& hint) 
				: Serialization::ItemSerializer(name, hint) {}

			virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, EntryState* target)const final override {
				EntryState& state = *target;
				if (state.shader != nullptr) {
					state.shaderPath = state.shader->LitShaderPath();
					state.editorPaths.resize(state.shader->EditorPathCount());
					for (size_t i = 0u; i < state.editorPaths.size(); i++)
						state.editorPaths[i] = state.shader->EditorPath(i);
					state.blendMode = (decltype(state.blendMode))state.shader->BlendMode();
					state.materialFlags = (decltype(state.materialFlags))state.shader->MaterialFlags();
					state.properties.resize(state.shader->PropertyCount());
					for (size_t i = 0u; i < state.properties.size(); i++)
						state.properties[i] = state.shader->Property(i);
					state.shadingStateSize = state.shader->ShadingStateSize();
				}
				JM_LitShaderSetSerializer_SERIALIZE_FIELD(state.shaderPath, "Shader Path", "Load-path for the shader");
				JM_LitShaderSetSerializer_SERIALIZE_FIELD(state.editorPaths, "Editor Paths", "Paths for the editor selector");
				JM_LitShaderSetSerializer_SERIALIZE_FIELD(state.blendMode, "Blend Mode", "Shader blending mode");
				JM_LitShaderSetSerializer_SERIALIZE_FIELD(state.materialFlags, "Material Flags",
					"Optional vertex input requirenments, as well as some other optimization and/or features");
				JM_LitShaderSetSerializer_SERIALIZE_FIELD(state.properties, "Material Properties", "Material Property fields");
				JM_LitShaderSetSerializer_SERIALIZE_FIELD(state.shadingStateSize, "Shading State Size", "JM_ShadingStateSize within the shader");
				if (state.dirty)
					state.shader = Object::Instantiate<Material::LitShader>(
						state.shaderPath, state.editorPaths,
						static_cast<Material::BlendMode>(state.blendMode),
						static_cast<Material::MaterialFlags>(state.materialFlags),
						state.shadingStateSize, state.properties);
			}
		};
		std::shared_ptr<const std::vector<Reference<const EntrySerializer>>> serializers = 
			[&]() -> std::shared_ptr<const std::vector<Reference<const EntrySerializer>>> {
			static std::shared_ptr<std::vector<Reference<const EntrySerializer>>> serializationBuffer;
			SpinLock bufferLock;
			std::unique_lock<SpinLock> lock(bufferLock);
			std::shared_ptr<const std::vector<Reference<const EntrySerializer>>> rv = serializationBuffer;
			if (rv == nullptr || rv->size() < state.count) {
				serializationBuffer = std::make_shared<std::vector<Reference<const EntrySerializer>>>();
				if (rv != nullptr)
					for (size_t i = 0u; i < rv->size(); i++)
						serializationBuffer->push_back(rv->operator[](i));
				while (serializationBuffer->size() < state.count) {
					std::stringstream stream;
					stream << serializationBuffer->size();
					const std::string name = stream.str();
					serializationBuffer->push_back(Object::Instantiate<EntrySerializer>(name, "Lit-Shader " + name));
				}
				rv = serializationBuffer;
			}
			return rv;
		}();
		std::set<Reference<const Material::LitShader>> shaders;
		for (size_t i = 0u; i < state.count; i++) {
			EntryState entryState = {};
			entryState.shader = ((*target) == nullptr || (*target)->Size() <= i) ? nullptr : (*target)->At(i);
			entryState.dirty = entryState.shader == nullptr;
			recordElement(serializers->operator[](i)->Serialize(entryState));
			state.dirty |= entryState.dirty;
			if (entryState.shader != nullptr)
				shaders.insert(entryState.shader);
		}

		// Create new set if dirty:
		if (state.dirty)
			(*target) = Object::Instantiate<Material::LitShaderSet>(shaders);

#undef JM_LitShaderSetSerializer_SERIALIZE_FIELD
	}
}
