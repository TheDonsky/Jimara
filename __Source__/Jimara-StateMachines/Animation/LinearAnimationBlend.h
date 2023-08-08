#pragma once
#include "AnimationBlendStateProvider.h"
#include <Jimara/Core/Systems/InputProvider.h>


namespace Jimara {
	/// <summary> Let the system know about the Component type </summary>
	JIMARA_REGISTER_TYPE(Jimara::LinearAnimationBlend);

#pragma warning(disable: 4250)
	/// <summary>
	/// AnimationBlendStateProvider that blends between several other AnimationBlendStateProvider objects based on some floating point input
	/// </summary>
	class JIMARA_STATE_MACHINES_API LinearAnimationBlend : public virtual Jimara::Component, public virtual AnimationBlendStateProvider {
	public:
		/// <summary>
		/// Blend tree node
		/// </summary>
		struct JIMARA_STATE_MACHINES_API ClipData {
			/// <summary> Animation Blend State Provider </summary>
			WeakReference<AnimationBlendStateProvider> animation;

			/// <summary> Playback speed multiplier </summary>
			WeakReference<Jimara::InputProvider<float>> playbackSpeedMultiplier;

			/// <summary> Input value for which the blend state is fully set to this animation </summary>
			float value = 0.0f;

			/// <summary> Serializer for ClipData </summary>
			struct Serializer;
		};

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		LinearAnimationBlend(Component* parent, const std::string_view& name = "LinearAnimationBlend");

		/// <summary> Virtual destructor </summary>
		virtual ~LinearAnimationBlend();

		/// <summary> Number of 'branches'/sub-AnimationBlendStateProviders in the blend tree </summary>
		inline size_t BranchCount()const { return m_clips.size(); }

		/// <summary>
		/// sub-AnimationBlendStateProvider by index
		/// </summary>
		/// <param name="index"> Branch index </param>
		/// <returns> Branch data </returns>
		inline const ClipData& Branch(size_t index)const { return m_clips[index]; }

		/// <summary>
		/// sub-AnimationBlendStateProvider by index
		/// </summary>
		/// <param name="index"> Branch index </param>
		/// <returns> Branch data </returns>
		inline ClipData& Branch(size_t index) { return m_clips[index]; }

		/// <summary>
		/// Adds sub-AnimationBlendStateProvider
		/// </summary>
		/// <param name="data"> Branch </param>
		inline void AddBranch(const ClipData& data) { m_clips.push_back(data); }

		/// <summary>
		/// Removed sub-AnimationBlendStateProvider by index
		/// </summary>
		/// <param name="index"> Branch index </param>
		inline void RemoveBranch(size_t index) { m_clips.erase(m_clips.begin() + index); }

		/// <summary> Base playback speed provider (multiplier) </summary>
		inline Reference<Jimara::InputProvider<float>> BasePlaybackSpeed()const { return m_playbackSpeed; }

		/// <summary>
		/// Sets base playback speed
		/// </summary>
		/// <param name="provider"> Speed multiplier </param>
		inline void SetBasePlaybackSpeed(Jimara::InputProvider<float>* provider) { m_playbackSpeed = Reference<Jimara::InputProvider<float>>(provider); }

		/// <summary>
		/// Exposes fields
		/// </summary>
		/// <param name="recordElement"> Fields will be exposed through this callback </param>
		virtual void GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement) override;

		/// <summary>
		/// Reports clip
		/// </summary>
		/// <param name="reportClipState"> Clip will be reported through this callback </param>
		virtual void GetBlendState(Callback<AnimationBlendStateProvider::ClipBlendState> reportClipState)override;


	private:
		// Clips to blend
		std::vector<ClipData> m_clips;

		// Base playback speed multiplier (will be multiplied by Speed multiplier)
		WeakReference<Jimara::InputProvider<float>> m_playbackSpeed;

		// Input value for blending
		WeakReference<Jimara::InputProvider<float>> m_value;
	};
#pragma warning(default: 4250)



	/// <summary>
	/// Serializer for ClipData
	/// </summary>
	struct JIMARA_STATE_MACHINES_API LinearAnimationBlend::ClipData::Serializer : public virtual Jimara::Serialization::SerializerList::From<ClipData> {
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="name"> Serializer name </param>
		/// <param name="hint"> Serialization hint </param>
		/// <param name="attributes"> Serializer attributes </param>
		inline Serializer(const std::string_view& name, const std::string_view& hint, const std::vector<Reference<const Object>>& attributes)
			: Jimara::Serialization::ItemSerializer(name, hint, attributes) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~Serializer() {}

		/// <summary>
		/// Reports serialized fields
		/// </summary>
		/// <param name="recordElement"> Each field will be reported through this callback </param>
		/// <param name="target"> Clip data </param>
		virtual void GetFields(const Callback<Jimara::Serialization::SerializedObject>& recordElement, ClipData* target)const final override;
	};



	// Expose parent types and attributes
	template<> inline void TypeIdDetails::GetParentTypesOf<Jimara::LinearAnimationBlend>(const Callback<TypeId>& report) {
		report(TypeId::Of<Component>());
		report(TypeId::Of<AnimationBlendStateProvider>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<Jimara::LinearAnimationBlend>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<Jimara::LinearAnimationBlend> serializer("Jimara/Animation/LinearAnimationBlend", "LinearAnimationBlend");
		report(&serializer);
	}
}
