#pragma once
#include "../Base/VectorInput.h"
#include <Jimara/Components/Physics/Rigidbody.h>


namespace Jimara {
	/// <summary> Let the system know about the type </summary>
	JIMARA_REGISTER_TYPE(Jimara::RigidbodyFieldInput);

	/// <summary>
	/// 3d input from Rigidbody component state
	/// </summary>
	class JIMARA_GENERIC_INPUTS_API RigidbodyFieldInput : public virtual VectorInput::ComponentFrom<Vector3> {
	public:
		/// <summary>
		/// Input value mode
		/// </summary>
		enum class InputMode : uint8_t {
			/// <summary> Movement speed vector </summary>
			VELOCITY = 0u,

			/// <summary> Rotation speed </summary>
			ANGULAR_VELOCITY = 1u,

			/// <summary> Vector3(Mass(), 0, 0) </summary>
			MASS = 2u,

			/// <summary> Vector3(CCDEnabled() ? 1 : 0, 0, 0) </summary>
			CCD_ENABLED = 3u,

			/// <summary> Gravitational acceleration constanty (does not need the source) </summary>
			GRAVITY = 4u,

			/// <summary> Nothing; No input, no value </summary>
			NO_INPUT = 5u
		};

		/// <summary> Input flags </summary>
		enum class InputFlags : uint8_t {
			/// <summary> No effect </summary>
			NONE = 0u,

			/// <summary> If true, input will not be produced if the input component is disabled in hierarchy </summary>
			NO_VALUE_IF_DISABLED = (1u << 0u),

			/// <summary> If set, this flag will force the source component to be found on parent components in case it's not set </summary>
			FIND_SOURCE_ON_PARENT_CHAIN_IF_NOT_SET = (1u << 1u)
		};

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		RigidbodyFieldInput(Component* parent, const std::string_view& name = "TransformInput");

		/// <summary> Virtual destructor </summary>
		virtual ~RigidbodyFieldInput();

		/// <summary>
		/// Source component for retrieving fields from
		/// <para/> If nullptr, as long as FIND_SOURCE_ON_PARENT_CHAIN_IF_NOT_SET flag is set, value will come from a component in parent chain.
		/// </summary>
		inline Rigidbody* Source()const { return m_source.operator Jimara::Reference<Jimara::Rigidbody>(); }

		/// <summary>
		/// Sets source component
		/// </summary>
		/// <param name="source"> Source component for retrieving fields from </param>
		inline void SetSource(Rigidbody* source) { m_source = source; }

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
		// Source rigidbody
		WeakReference<Rigidbody> m_source;

		// Mode
		InputMode m_mode = InputMode::VELOCITY;

		// Flags
		InputFlags m_flags = static_cast<InputFlags>(
			static_cast<std::underlying_type_t<InputFlags>>(InputFlags::NO_VALUE_IF_DISABLED) |
			static_cast<std::underlying_type_t<InputFlags>>(InputFlags::FIND_SOURCE_ON_PARENT_CHAIN_IF_NOT_SET));
	};


	// Define boolean operators for InputFlags:
	JIMARA_DEFINE_ENUMERATION_BOOLEAN_OPERATIONS(Jimara::RigidbodyFieldInput::InputFlags);

	// Expose type details:
	template<> inline void TypeIdDetails::GetParentTypesOf<RigidbodyFieldInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<VectorInput::ComponentFrom<Vector3>>());
	}
	template<> JIMARA_GENERIC_INPUTS_API void TypeIdDetails::GetTypeAttributesOf<RigidbodyFieldInput>(const Callback<const Object*>& report);
}
