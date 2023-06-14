#pragma once
#include "../Component.h"
#include "../../Environment/Rendering/PostFX/Tonemapper/TonemapperKernel.h"


namespace Jimara {
	/// <summary> This will make sure, Tonemapper is registered with BuiltInTypeRegistrator </summary>
	JIMARA_REGISTER_TYPE(Jimara::Tonemapper);

	/// <summary>
	/// Tonemapper post-processing effect
	/// </summary>
	class JIMARA_API Tonemapper : public virtual Component {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Name of the camera component </param>
		Tonemapper(Component* parent, const std::string_view& name = "Tonemapper");

		/// <summary> Virtual destructor </summary>
		virtual ~Tonemapper();

		/// <summary> Tonemapper type </summary>
		TonemapperKernel::Type Type()const;

		/// <summary>
		/// Sets tonemapper type
		/// </summary>
		/// <param name="type"> Tonemapping algorithm </param>
		void SetType(TonemapperKernel::Type type);

		/// <summary> 
		/// Renderer category for render stack 
		/// Note: Higher category will render later; refer to Scene::GraphicsContext::Renderer for further details.
		/// </summary>
		uint32_t RendererCategory()const;

		/// <summary>
		/// Sets renderer category for render stack
		/// Note: Higher category will render later; refer to Scene::GraphicsContext::Renderer for further details.
		/// </summary>
		/// <param name="category"> Category to set </param>
		void SetRendererCategory(uint32_t category);

		/// <summary> 
		/// Renderer priority for render stack 
		/// Note: Higher priority will render earlier within the same category; refer to Scene::GraphicsContext::Renderer for further details.
		/// </summary>
		uint32_t RendererPriority()const;

		/// <summary>
		/// Sets renderer priority for render stack
		/// Note: Higher priority will render earlier within the same category; refer to Scene::GraphicsContext::Renderer for further details.
		/// </summary>
		/// <param name="priority"> Priority to set </param>
		void SetRendererPriority(uint32_t priority);

		/// <summary>
		/// Exposes fields to serialization utilities
		/// </summary>
		/// <param name="recordElement"> Reports elements with this </param>
		virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override;

	protected:
		/// <summary> Invoked, whenever the component becomes recognized by engine </summary>
		virtual void OnComponentInitialized()override;

		/// <summary> Invoked, whenever the component becomes active in herarchy </summary>
		virtual void OnComponentEnabled()override;

		/// <summary> Invoked, whenever the component stops being active in herarchy </summary>
		virtual void OnComponentDisabled()override;

		/// <summary> Invoked, whenever the component gets destroyed </summary>
		virtual void OnComponentDestroyed()override;

	private:
		// Tonemapper type
		TonemapperKernel::Type m_type = TonemapperKernel::Type::REINHARD_PER_CHANNEL;

		// Settings for TonemapperKernel::Type::REINHARD_EX
		TonemapperKernel::ReinhardPerChannelSettings m_reinhardSettings;

		// Renderer category for render stack (higher category will render later)
		uint32_t m_category = 1024;

		// Renderer priority for render stack (higher priority will render earlier within the same category)
		uint32_t m_priority = 0;
		
		// Render stack, this camera renders to
		Reference<RenderStack> m_renderStack;

		// Underlying RenderStack renderer
		Reference<RenderStack::Renderer> m_renderer;

		// Some private stuff resides here
		struct Helpers;
	};


	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<Tonemapper>(const Callback<TypeId>& report) { report(TypeId::Of<Component>()); }
	template<> void TypeIdDetails::GetTypeAttributesOf<Tonemapper>(const Callback<const Object*>& report);
}
