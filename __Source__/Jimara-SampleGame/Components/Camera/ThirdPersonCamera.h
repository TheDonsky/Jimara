#pragma once
#include <Jimara/Components/Camera.h>


namespace Jimara {
	namespace SampleGame {
		/// <summary> Register controller class </summary>
		JIMARA_REGISTER_TYPE(Jimara::SampleGame::ThirdPersonCamera);

		/// <summary>
		/// A sample for how a third person camera can be implemented
		/// <para/> Expectation is that ThirdPersonCamera will have a valid Camera component in parent hierarchy and said Camera will have a valid Transform we can control.
		/// </summary>
		class ThirdPersonCamera : public virtual Component {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="parent"> Parent component </param>
			/// <param name="name"> 
			///		ThirdPersonCamera controller component name (note that this one HAS TO HAVE a default value, or else ComponentSerializer will not compile) 
			/// </param>
			ThirdPersonCamera(Component* parent, const std::string_view& name = "ThirdPersonCamera");

			/// <summary> Virtual destructor </summary>
			virtual ~ThirdPersonCamera();

			/// <summary>
			/// Override for Jimara::Serialization::Serializable to expose controller settings
			/// </summary>
			/// <param name="reportItem"> Callback, with which the serialization utilities access fields </param>
			virtual void GetFields(Callback<Serialization::SerializedObject> reportItem)override;

		protected:
			/// <summary>
			/// Invoked each time the component gets enabled
			/// <para/> Note: Since we do not use standard update routines here, this will help us to subscribe to the update cycle
			/// </summary>
			virtual void OnComponentEnabled()override;

			/// <summary>
			/// Invoked each time the component gets disabled
			/// <para/> Note: Since we do not use standard update routines here, this will help us to unsubscribe from the update cycle
			/// </summary>
			virtual void OnComponentDisabled()override;

		private:
			// Transform, the camera will be looking at
			Reference<Transform> m_targetTransform;

			// Point on screen, on which the target transform position will appear (range [-0.5f, 0.5f] is mapped from left to right and/or bottom to top)
			Vector2 m_targetOnScreenPosition = Vector2(0.0f);

			// Minimal and maximal camera pitch (valid range is (-90, 90))
			float m_minPicth = -60.0f;
			float m_maxPitch = 80.0f;

			// Distance to target
			float m_targetDistance = 4.0f;

			// 'Zoom-in/out' speed
			float m_zoomInSpeed = 2.0f;
			float m_zoomOutSpeed = 1.0f;

			// I arbitrarily decided to hide some of the private methods under this scope
			class Helpers;
		};
	}

	// TypeIdDetails::GetTypeAttributesOf exposes the serializer
	template<> void TypeIdDetails::GetTypeAttributesOf<SampleGame::ThirdPersonCamera>(const Callback<const Object*>& report);
}
