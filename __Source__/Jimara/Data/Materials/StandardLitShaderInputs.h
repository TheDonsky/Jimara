#include "../../Graphics/GraphicsDevice.h"


namespace Jimara {
	/// <summary>
	/// Standard Lit shader input names and utilities
	/// </summary>
	class JIMARA_API StandardLitShaderInputs {
	public:
		/// <summary> Mesh Vertex position (vertex shader input name) </summary>
		inline static constexpr std::string_view JM_VertexPosition_Name = "JM_VertexPosition";

		/// <summary> Mesh Vertex position location (vertex shader input index) </summary>
		inline static constexpr uint32_t JM_VertexPosition_Location = 0u;

		/// <summary> Mesh Vertex normal (vertex shader input name) </summary>
		inline static constexpr std::string_view JM_VertexNormal_Name = "JM_VertexNormal";

		/// <summary> Mesh Vertex normal location (vertex shader input index) </summary>
		inline static constexpr uint32_t JM_VertexNormal_Location = 1u;

		/// <summary> Mesh Vertex UV coordinate (vertex shader input name) </summary>
		inline static constexpr std::string_view JM_VertexUV_Name = "JM_VertexUV";

		/// <summary> Mesh Vertex UV coordinate location (vertex shader input index) </summary>
		inline static constexpr uint32_t JM_VertexUV_Location = 2u;


		/// <summary> Mesh to world-space transformation matrix (vertex shader input name) </summary>
		inline static constexpr std::string_view JM_ObjectTransform_Name = "JM_ObjectTransform";

		/// <summary> Mesh to world-space transformation matrix location (vertex shader input index) </summary>
		inline static constexpr uint32_t JM_ObjectTransform_Location = 3u;


		/// <summary> Vertex color (vertex shader input name) </summary>
		inline static constexpr std::string_view JM_VertexColor_Name = "JM_VertexColor";

		/// <summary> Vertex color (vertex shader input location) </summary>
		inline static constexpr uint32_t JM_VertexColor_Location = 7u;


		/// <summary> Per object/instance tiling and offset for JM_VertexUV (vertex shader input name) </summary>
		inline static constexpr std::string_view JM_ObjectTilingAndOffset_Name = "JM_ObjectTilingAndOffset";

		/// <summary> Per object/instance tiling and offset for JM_VertexUV (vertex shader input location) </summary>
		inline static constexpr uint32_t JM_ObjectTilingAndOffset_Location = 8u;


		/// <summary> Object/instance index within the graphics descriptor (vertex shader input name) </summary>
		inline static constexpr std::string_view JM_ObjectIndex_Name = "JM_ObjectIndex";

		/// <summary> Object/instance index within the graphics descriptor (vertex shader input location) </summary>
		inline static constexpr uint32_t JM_ObjectIndex_Location = 9u;


	private:
		inline StandardLitShaderInputs() {}
	};
}
