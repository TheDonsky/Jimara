#pragma once
#include "../GizmoScene.h"

namespace Jimara {
	namespace Editor {
		/// <summary> Let the system know TransformHandleSettings exist </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::TransformHandleSettings);

		/// <summary>
		/// 'Global' settings that can be used by move/rotate/scale type elements
		/// </summary>
		class TransformHandleSettings : public virtual ObjectCache<Reference<const Object>>::StoredObject {
		public:
			/// <summary>
			/// Retrieves common instance of TransformHandleSettings for given Editor context
			/// </summary>
			/// <param name="context"> Editor application context </param>
			/// <returns> Shared TransformHandleSettings instance </returns>
			static Reference<TransformHandleSettings> Of(EditorContext* context);

			/// <summary>
			/// Retrieves common instance of TransformHandleSettings for given Gizmo scene context
			/// </summary>
			/// <param name="context"> Gizmo scene context </param>
			/// <returns> Shared TransformHandleSettings instance </returns>
			inline static Reference<TransformHandleSettings> Of(GizmoScene::Context* context) { return Of(context->EditorApplicationContext()); }

			/// <summary>
			/// Active handle type (MOVE/ROTATE/SCLE)
			/// </summary>
			enum class HandleType : uint8_t {
				/// <summary> Movement 'arrows' </summary>
				MOVE = 0,

				/// <summary> Rotation 'toruses' </summary>
				ROTATE = 1,

				/// <summary> Scale 'arrows' </summary>
				SCALE = 2
			};

			/// <summary>
			/// Tells, whether to place the handles in world or local space (LOCAL/WORLD)
			/// </summary>
			enum class AxisSpace : uint8_t {
				/// <summary> Transformation handles should be rotated the same as target, when possible/applicable </summary>
				LOCAL = 0,

				/// <summary> Transformation handles should be world-aligned (ei not rotated) </summary>
				WORLD = 1
			};

			/// <summary>
			/// Tells, what to use as the pivot point during scale/rotation 
			/// (ei, whether to rotate around or scale out of "averaged-out" center point or to deal with the individual origins)
			/// </summary>
			enum class PivotMode : uint8_t {
				/// <summary> Scale/Rotation should be done 'around' the averaged-out pivot point, when applicable </summary>
				AVERAGE = 0,

				/// <summary> Scale/Roation should be done at the individual origin points </summary>
				INDIVIDUAL = 1
			};

			/// <summary> Active handle type (MOVE/ROTATE/SCLE) </summary>
			inline HandleType HandleMode()const { return m_handleType; }

			/// <summary>
			/// Sets active handle type
			/// </summary>
			/// <param name="type"> MOVE/ROTATE/SCLE </param>
			inline void SetHandleMode(HandleType type) { m_handleType = Math::Min(Math::Max(HandleType::MOVE, type), HandleType::SCALE); }

			/// <summary> Tells, whether to place the handles in world or local space (LOCAL/WORLD) </summary>
			inline AxisSpace HandleOrientation()const { return m_axisSpace; }

			/// <summary>
			/// Sets handle orientation
			/// </summary>
			/// <param name="space"> LOCAL/WORLD </param>
			inline void SetHandleOrientation(AxisSpace space) { m_axisSpace = Math::Min(Math::Max(AxisSpace::LOCAL, space), AxisSpace::WORLD); }

			/// <summary> Tells, what to use as the pivot point during scale/rotation (AVERAGE/INDIVIDUAL) </summary>
			inline PivotMode PivotPosition()const { return m_pivotMode; }

			/// <summary>
			/// Sets pivot mode
			/// </summary>
			/// <param name="mode"> AVERAGE/INDIVIDUAL </param>
			inline void SetPivotPosition(PivotMode mode) { m_pivotMode = Math::Min(Math::Max(PivotMode::AVERAGE, mode), PivotMode::INDIVIDUAL); }

		private:
			// HandleMode
			std::atomic<HandleType> m_handleType = HandleType::MOVE;

			// HandleOrientation
			std::atomic<AxisSpace> m_axisSpace = AxisSpace::LOCAL;

			// PivotPosition
			std::atomic<PivotMode> m_pivotMode = PivotMode::AVERAGE;

			// Cache of instances
			class Cache;

			// Constructor needs to be private
			inline TransformHandleSettings() {}
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::OnRegisterType<Editor::TransformHandleSettings>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::TransformHandleSettings>();
	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::TransformHandleSettings>(const Callback<const Object*>& report);
}
