#pragma once
#include "../../SceneObjects/Objects/GraphicsObjectDescriptor.h"


namespace Jimara {
	/// <summary>
	/// All standard vertex input fields as buffer-device-addresses:
	/// <para/> Size: 112, alighnment 8.
	/// </summary>
	struct JM_StandardVertexInput final {
		/// <summary> Per-field flags </summary>
		enum class Flags : uint32_t {
			/// <summary> Empty bitmask </summary>
			NONE = 0,

			/// <summary> Graphics::GraphicsPipeline::VertexInputInfo::InputRate::VERTEX </summary>
			FIELD_RATE_PER_VERTEX = (1 << 0),

			/// <summary> Graphics::GraphicsPipeline::VertexInputInfo::InputRate::INSTANCE </summary>
			FIELD_RATE_PER_INSTANCE = (1 << 1),
		};
		static_assert(sizeof(Flags) == sizeof(uint32_t));
		static_assert(alignof(Flags) == 4u);

		/// <summary>
		/// Vertex-input field buffer with frequency bits and stride
		/// <para/> Size: 16, alighnment 8.
		/// </summary>
		struct Field final {
			/// <summary> Buffer-Device-Address </summary>
			alignas(8) uint64_t buffId = 0u;

			/// <summary> Buffer-Element stride </summary>
			alignas(4) uint32_t elemStride = 0u;

			/// <summary> Field flags </summary>
			alignas(4) Flags flags = Flags::NONE;
		};
		static_assert(sizeof(Field) == 16u);
		static_assert(alignof(Field) == 8u);



		/// <summary> Helper for extracting data from graphics objects </summary>
		class Extractor;

		/// <summary> Field-Binding information </summary>
		struct FieldBinding;



		/// <summary> JM_VertexPosition </summary>
		alignas(8) Field vertexPosition = {};

		/// <summary> JM_VertexNormal </summary>
		alignas(8) Field vertexNormal = {};

		/// <summary> JM_VertexUV </summary>
		alignas(8) Field vertexUV = {};

		/// <summary> JM_VertexColor </summary>
		alignas(8) Field vertexColor = {};

		/// <summary> JM_ObjectTransform </summary>
		alignas(8) Field objectTransform = {};

		/// <summary> JM_ObjectTilingAndOffset </summary>
		alignas(8) Field objectTilingAndOffset = {};

		/// <summary> JM_ObjectIndex </summary>
		alignas(8) Field objectIndex = {};
	};
	static_assert(sizeof(JM_StandardVertexInput) == 112);
	static_assert(alignof(JM_StandardVertexInput) == 8u);

	JIMARA_DEFINE_ENUMERATION_BOOLEAN_OPERATIONS(JM_StandardVertexInput::Flags);



	/// <summary> Field-Binding information </summary>
	struct JM_StandardVertexInput::FieldBinding {
		/// <summary> Buffer binding </summary>
		Reference<const Graphics::ResourceBinding<Graphics::ArrayBuffer>> bufferBinding;

		/// <summary> Buffer-Element stride </summary>
		uint32_t elemStride = 0u;

		/// <summary> Buffer-Element offset </summary>
		uint32_t elemOffset = 0u;

		/// <summary> Field flags </summary>
		Flags flags = Flags::NONE;

		/// <summary> Translates FieldBinding to field </summary>
		inline Field GetField(std::vector<Reference<const Object>>* resourceList)const {
			Field res = {};
			res.buffId = 0u;
			if (bufferBinding != nullptr) {
				Graphics::ArrayBuffer* buffer = bufferBinding->BoundObject();
				if (buffer != nullptr) {
					res.buffId = buffer->DeviceAddress() + elemOffset;
					if (resourceList != nullptr)
						resourceList->push_back(buffer);
				}
			}
			res.elemStride = elemStride;
			res.flags = flags;
			return res;
		}
	};


	/// <summary> Helper for extracting data from graphics objects </summary>
	class JM_StandardVertexInput::Extractor final {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="data"> Viewport-Data, used as the source </param>
		/// <param name="logger"> Optional logger for error reporting </param>
		Extractor(const GraphicsObjectDescriptor::ViewportData* data = nullptr, OS::Logger* logger = nullptr);

		/// <summary> Destructor </summary>
		~Extractor();

		/// <summary> 
		/// Extracts JM_StandardVertexInput 
		/// <para/> Note that IndexBuffer will not be stored inside the resourceList.
		/// </summary>
		/// <param name="resourceList"> [optional] List of resources to append extracted resources to </param>
		/// <returns> vertex input </returns>
		JM_StandardVertexInput Get(std::vector<Reference<const Object>>* resourceList)const;

		/// <summary> Viewport-Data, used as the source </summary>
		inline const GraphicsObjectDescriptor::ViewportData* Source()const { return m_data; }

		/// <summary> JM_VertexPosition </summary>
		inline const FieldBinding& VertexPosition()const { return m_vertexPosition; }

		/// <summary> JM_VertexNormal </summary>
		inline const FieldBinding& VertexNormal()const { return m_vertexNormal; }

		/// <summary> JM_VertexUV </summary>
		inline const FieldBinding& VertexUV()const { return m_vertexUV; }

		/// <summary> JM_VertexColor </summary>
		inline const FieldBinding& VertexColor()const { return m_vertexColor; }

		/// <summary> JM_ObjectTransform </summary>
		inline const FieldBinding& ObjectTransform()const { return m_objectTransform; }

		/// <summary> JM_ObjectTilingAndOffset </summary>
		inline const FieldBinding& ObjectTilingAndOffset()const { return m_objectTilingAndOffset; }

		/// <summary> JM_ObjectIndex </summary>
		inline const FieldBinding& ObjectIndex()const { return m_objectIndex; }

		/// <summary> Index buffer binding </summary>
		inline const Graphics::ResourceBinding<Graphics::ArrayBuffer>* IndexBuffer()const { return m_indexBuffer; }


	private:
		Reference<const GraphicsObjectDescriptor::ViewportData> m_data;
		FieldBinding m_vertexPosition;
		FieldBinding m_vertexNormal;
		FieldBinding m_vertexUV;
		FieldBinding m_vertexColor;
		FieldBinding m_objectTransform;
		FieldBinding m_objectTilingAndOffset;
		FieldBinding m_objectIndex;
		Reference<const Graphics::ResourceBinding<Graphics::ArrayBuffer>> m_indexBuffer;
	};
}
