#include "JM_StandardVertexInputStructure.h"
#include "../../../../Data/Materials/StandardLitShaderInputs.h"


namespace Jimara {
	JM_StandardVertexInput::Extractor::Extractor(const GraphicsObjectDescriptor::ViewportData* data, OS::Logger* logger)
		: m_data(data) {
		if (m_data == nullptr)
			return;
		const GraphicsObjectDescriptor::VertexInputInfo& vertexInput = data->VertexInput();

		for (size_t bufferId = 0u; bufferId < vertexInput.vertexBuffers.Size(); bufferId++) {
			const GraphicsObjectDescriptor::VertexBufferInfo& bufferInfo = vertexInput.vertexBuffers[bufferId];

			auto populateFieldBinding = [&](FieldBinding& binding, const std::string_view& name) {
				if (logger != nullptr)
					logger->Warning("JM_StandardVertexInput::Extractor::Extractor - ", name, " binding encountered more than once!");
				binding.bufferBinding = bufferInfo.binding;
				binding.elemStride = static_cast<uint32_t>(bufferInfo.layout.bufferElementSize);
				binding.flags =
					(bufferInfo.layout.inputRate == Graphics::GraphicsPipeline::VertexInputInfo::InputRate::VERTEX) ? Flags::FIELD_RATE_PER_VERTEX :
					(bufferInfo.layout.inputRate == Graphics::GraphicsPipeline::VertexInputInfo::InputRate::INSTANCE) ? Flags::FIELD_RATE_PER_INSTANCE :
					Flags::NONE;
			};

			for (size_t locationId = 0u; locationId < bufferInfo.layout.locations.Size(); locationId++) {
				const Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo& location = bufferInfo.layout.locations[locationId];
#define JM_StandardVertexInput_SetFieldBinding(JM_location, binding)  \
				if ((location.location.has_value() && location.location.value() == StandardLitShaderInputs::JM_location##_Location) || \
					(location.name == StandardLitShaderInputs::JM_location##_Name)) { \
					populateFieldBinding(binding, #JM_location); \
					continue; \
				}

				JM_StandardVertexInput_SetFieldBinding(JM_VertexPosition, m_vertexPosition);
				JM_StandardVertexInput_SetFieldBinding(JM_VertexNormal, m_vertexNormal);
				JM_StandardVertexInput_SetFieldBinding(JM_VertexUV, m_vertexUV);
				JM_StandardVertexInput_SetFieldBinding(JM_VertexColor, m_vertexColor);
				JM_StandardVertexInput_SetFieldBinding(JM_ObjectTransform, m_objectTransform);
				JM_StandardVertexInput_SetFieldBinding(JM_ObjectTilingAndOffset, m_objectTilingAndOffset);
				JM_StandardVertexInput_SetFieldBinding(JM_ObjectIndex, m_objectIndex);

#undef JM_StandardVertexInput_SetFieldBinding
			}

			m_indexBuffer = vertexInput.indexBuffer;
		}
	}

	JM_StandardVertexInput::Extractor::~Extractor() {}

	JM_StandardVertexInput JM_StandardVertexInput::Extractor::Get(std::vector<Reference<const Object>>* resourceList)const {
		JM_StandardVertexInput res = {};
		res.vertexPosition = m_vertexPosition.GetField(resourceList);
		res.vertexNormal = m_vertexNormal.GetField(resourceList);
		res.vertexUV = m_vertexUV.GetField(resourceList);
		res.vertexColor = m_vertexColor.GetField(resourceList);
		res.objectTransform = m_objectTransform.GetField(resourceList);
		res.objectTilingAndOffset = m_objectTilingAndOffset.GetField(resourceList);
		res.objectIndex = m_objectIndex.GetField(resourceList);
		return res;
	}
}

