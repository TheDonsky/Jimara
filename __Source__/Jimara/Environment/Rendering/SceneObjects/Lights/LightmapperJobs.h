#pragma once
#include "LightDescriptor.h"


namespace Jimara {
	/// <summary>
	/// Scene renderers need to be executed after the shadows are baked and miscelenious lightmapping tasks are complete.
	/// LightmapperJobs is a generic container that stores and maintains the collection of all active lightmappers and can report them
	/// to the renderers as dependencies.
	/// </summary>
	class JIMARA_API LightmapperJobs : public virtual Object {
	public:
		/// <summary>
		/// Lightmapper job can be any job derived from LightmapperJobs::Job class
		/// </summary>
		class JIMARA_API Job : public virtual JobSystem::Job {
		public:
			/// <summary>
			/// LightmapperJobs will flush on Scene::GraphicsContext::OnGraphicsSynch
			/// </summary>
			/// <param name="context"> Scene context </param>
			/// <returns> Scene::GraphicsContext::OnGraphicsSynch </returns>
			inline static Event<>& OnFlushSceneObjectCollections(SceneContext* context) { return context->Graphics()->OnGraphicsSynch(); }
		};

		/// <summary>
		/// Owner of the stored item
		/// <para/> Note: Refer to SceneObjectCollection for further details
		/// </summary>
		using ItemOwner = SceneObjectCollection<Job>::ItemOwner;

		/// <summary>
		/// Gets shared instance for the context's main LightDescriptor set
		/// </summary>
		/// <param name="context"> Scene context </param>
		/// <returns> Shared LightmapperJobs </returns>
		static Reference<LightmapperJobs> GetInstance(SceneContext* context);

		/// <summary>
		/// Gets shared instance for given LightDescriptor set
		/// </summary>
		/// <param name="context"> Scene context </param>
		/// <returns> Shared LightmapperJobs </returns>
		static Reference<LightmapperJobs> GetInstance(LightDescriptor::Set* lightSet);

		/// <summary> Virtual destructor </summary>
		inline virtual ~LightmapperJobs() {}

		/// <summary>
		/// Adds job to the set
		/// </summary>
		/// <param name="item"> Job to add </param>
		inline void Add(ItemOwner* item) { m_set->Add(item); }

		/// <summary>
		/// Removes job from the set
		/// </summary>
		/// <param name="item"> Job to remove </param>
		inline void Remove(ItemOwner* item) { m_set->Remove(item); }

		/// <summary>
		/// Reports all items currently stored inside the collection
		/// <para/> Note: Content and behaviour is updated on Job::OnFlushSceneObjectCollections(context) event exclusively.
		/// </summary>
		/// <typeparam name="ReportObjectFn"> Any function/callback, that can receive Job*-s as arguments </typeparam>
		/// <param name="reportObject"> Each stored item will be reported through this callback </param>
		template<typename ReportObjectFn>
		inline void GetAll(const ReportObjectFn& reportObject)const { m_set->GetAll(reportObject); }

	private:
		// Underlying set
		const Reference<SceneObjectCollection<Job>> m_set;

		// Constructor is private
		inline LightmapperJobs(SceneContext* context) 
			: m_set(Object::Instantiate<SceneObjectCollection<Job>>(context)) {}
	};
}
