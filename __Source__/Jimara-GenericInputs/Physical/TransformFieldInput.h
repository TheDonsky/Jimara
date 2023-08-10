#pragma once
#include "../Base/VectorInput.h"
#include <Jimara/Components/Transform.h>


namespace Jimara {
	/// <summary> Let the system know about the type </summary>
	JIMARA_REGISTER_TYPE(Jimara::TransformFieldInput);

	/// <summary>
	/// 3d input from Transform component state
	/// </summary>
	class JIMARA_GENERIC_INPUTS_API TransformFieldInput : public virtual VectorInput::ComponentFrom<Vector3> {
	public:
		/// <summary>
		/// Input value mode
		/// </summary>
		enum class InputMode : uint8_t {
			/// <summary> World-space position </summary>
			WORLD_POSITION = 0u,

			/// <summary> Parent-space position </summary>
			LOCAL_POSITION = 1u,

			/// <summary> World-space euler angles </summary>
			WORLD_ROTATION = 2u,

			/// <summary> Parent-space euler angles </summary>
			LOCAL_ROTATION = 3u,

			/// <summary> Lossy(global) scale </summary>
			WORLD_SCALE = 4u,

			/// <summary> Local(parent-space) scale </summary>
			LOCAL_SCALE = 5u,

			/// <summary> World-space forward direction </summary>
			FORWARD = 6u,

			/// <summary> Parent-space forward direction </summary>
			LOCAL_FORWARD = 7u,

			/// <summary> World-space right direction </summary>
			RIGHT = 8u,

			/// <summary> Parent-space right direction </summary>
			LOCAL_RIGHT = 9u,

			/// <summary> World-space up direction </summary>
			UP = 10u,

			/// <summary> Parent-space right direction </summary>
			LOCAL_UP = 11u,

			/// <summary> Nothing; No input, no value </summary>
			NO_INPUT = 12u
		};

		/// <summary> Input flags </summary>
		enum class InputFlags : uint8_t {
			/// <summary> No effect </summary>
			NONE = 0u,

			/// <summary> If true, input will not be produced if the input component is disabled in hierarchy </summary>
			NO_VALUE_IF_DISABLED = (1u << 0u)
		};

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		TransformFieldInput(Component* parent, const std::string_view& name = "TransformInput");

		/// <summary> Virtual destructor </summary>
		virtual ~TransformFieldInput();

		/// <summary> Input mode </summary>
		inline InputMode Mode()const { return m_mode; }

		/// <summary>
		/// Sets input mode
		/// </summary>
		/// <param name="mode"> Input mode </param>
		inline void SetMode(InputMode mode) { m_mode = Math::Min(mode, InputMode::NO_INPUT); }

		/// <summary> Input flags/settings </summary>
		inline InputFlags Flags()const { return m_flags; }

		/// <summary>
		/// Sets input flags
		/// </summary>
		/// <param name="flags"> Settings </param>
		inline void SetFlags(InputFlags flags) { m_flags = flags; }

		/// <summary>
		/// Exposes fields
		/// </summary>
		/// <param name="recordElement"> Fields will be exposed through this callback </param>
		virtual void GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement)override;

		/// <summary> Evaluates input </summary>
		virtual std::optional<Vector3> EvaluateInput()override;

	private:
		// Mode
		InputMode m_mode = InputMode::WORLD_POSITION;

		// Flags
		InputFlags m_flags = InputFlags::NO_VALUE_IF_DISABLED;
	};

	// Expose type details:
	template<> inline void TypeIdDetails::GetParentTypesOf<TransformFieldInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<VectorInput::ComponentFrom<Vector3>>());
	}
	template<> JIMARA_GENERIC_INPUTS_API void TypeIdDetails::GetTypeAttributesOf<TransformFieldInput>(const Callback<const Object*>& report);
}
