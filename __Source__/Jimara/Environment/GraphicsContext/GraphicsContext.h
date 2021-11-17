#pragma once
namespace Jimara { class GraphicsContext; }
#include "../../Graphics/GraphicsDevice.h"
#include "../../Graphics/Data/GraphicsMesh.h"
#include "../../Graphics/Data/ShaderBinaries/ShaderLoader.h"
#include "Lights/LightDescriptor.h"
#include "SceneObjects/GraphicsObjectDescriptor.h"
#include "LightingModels/LightingModel.h"
#include <shared_mutex>


namespace Jimara {
	/// <summary>
	/// Object, describing scene graphics
	/// </summary>
	class GraphicsContext : public virtual Object {
	private:
		// Graphics device
		const Reference<Graphics::GraphicsDevice> m_device;

		// Shader loader
		const Reference<Graphics::ShaderLoader> m_shaderLoader;

		// Shader cache for shader reuse
		const Reference<Graphics::ShaderCache> m_shaderCache;

		// Mesh buffer cache
		const Reference<Graphics::GraphicsMeshCache> m_meshCache;

		// Components/renderers may use ReadLock with this, graphics synch point is generally supposed to be using WriteLock
		mutable std::shared_mutex m_lock;

	public:
		/// <summary>
		/// Aquiring this should be a guarantee, that the "Graphics snapshot" of the scene stays intact while we hold the lock.
		/// Note:
		///		If any renderer needs to interact with scene graphics objects, it should aquire the lock.
		///		If the read lock is not aquired during render, graphics synch point may happen mid-render and the resuling image might be scewd because of that...
		/// </summary>
		class ReadLock : public std::shared_lock<std::shared_mutex> {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Graphics context </param>
			inline ReadLock(const GraphicsContext& context) : std::shared_lock<std::shared_mutex>(context.m_lock) {}

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Graphics context </param>
			inline ReadLock(const GraphicsContext* context) : ReadLock(*context) {}
		};


	protected:
		/// <summary>
		/// Aquiring this prevents ReadLock-s to be created, so the graphics synch point implementation is expected to aquire this one to make sure, 
		/// the renderers do not spit out half-updated scenes or worse.
		/// Note: 
		///		Only the internal implementation of graphics context synch point is expected to use this. 
		///		Because of that, we made the class proiitected to make it inaccessible to regular pipelines and components.
		/// </summary>
		class WriteLock : public std::unique_lock<std::shared_mutex> {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Graphics context </param>
			inline WriteLock(GraphicsContext& context) : std::unique_lock<std::shared_mutex>(context.m_lock) {}

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Graphics context </param>
			inline WriteLock(GraphicsContext* context) : WriteLock(*context) {}
		};

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="device"> Graphics device </param>
		/// <param name="shaderLoader"> Shader loader </param>
		/// <param name="shaderCache"> Shader cache </param>
		/// <param name="meshCache"> Mesh buffer cache </param>
		GraphicsContext(Graphics::GraphicsDevice* device, Graphics::ShaderLoader* shaderLoader, Graphics::ShaderCache* shaderCache, Graphics::GraphicsMeshCache* meshCache)
			: m_device(device), m_shaderLoader(shaderLoader)
			, m_shaderCache((shaderCache == nullptr) ? Graphics::ShaderCache::ForDevice(device) : Reference<Graphics::ShaderCache>(shaderCache))
			, m_meshCache((meshCache == nullptr) ? Graphics::GraphicsMeshCache::ForDevice(device) : Reference<Graphics::GraphicsMeshCache>(meshCache)) { }



	public:
		/// <summary>
		/// Objects added to the grahics context may use callback provided by this class to synchronize their data with the scene
		/// Note: OnGraphicsSynch does not provide any thread-safety guarantees during execution (actual update is expected to be multithreaded), 
		///		so modifying scene cmponent data during execution is considered unsafe and is highly discuraged.
		/// </summary>
		class GraphicsObjectSynchronizer : public virtual Object {
		public:
			/// <summary>
			/// Invoked each graphics synch pooint and is expected to update the graphics representation of the target component(s)
			/// Notes:
			///		0. This callback does not provide any thread-safety guarantees during execution (actual update is expected to be multithreaded), 
			///		so modifying scene cmponent data during execution is considered unsafe and is highly discuraged.
			///		1. When the callback is invoked, graphics context's WriteLock will be pre-aquired, so taking ReadLock will result in crash and/or a deadlock (just don't do it).
			/// </summary>
			virtual void OnGraphicsSynch() = 0;
		};


		/// <summary> Graphics device </summary>
		inline Graphics::GraphicsDevice* Device()const { return m_device; }

		/// <summary> Shader loader </summary>
		inline Graphics::ShaderLoader* ShaderBytecodeLoader()const { return m_shaderLoader; }

		/// <summary> Device logger </summary>
		inline OS::Logger* Log()const { return m_device->Log(); }

		/// <summary> Shader cache </summary>
		inline Graphics::ShaderCache* ShaderCache()const { return m_shaderCache; }

		/// <summary> Mesh buffer cache </summary>
		inline Graphics::GraphicsMeshCache* MeshCache()const { return m_meshCache; }

		/// <summary> Invoked after each graphics synch point </summary>
		virtual Event<>& OnPostGraphicsSynch() = 0;

		/// <summary> Default lighting model for the GraphicsContext </summary>
		virtual LightingModel* DefaultLightingModel()const = 0;


		/// <summary>
		/// Schedules the object to be added to the graphics context for the next graphics synch point
		/// </summary>
		/// <param name="descriptor"> Graphics object descriptor to add </param>
		virtual void AddSceneObject(GraphicsObjectDescriptor* descriptor) = 0;

		/// <summary>
		/// Schedules the object to be removed from the graphics context for the next graphics synch point
		/// </summary>
		/// <param name="descriptor"> Graphics object to remove </param>
		virtual void RemoveSceneObject(GraphicsObjectDescriptor* descriptor) = 0;

		/// <summary> 
		/// Invoked, whenever the scene graphics objects get added 
		/// Notes:
		///		0. AddSceneObject does not directly trigger this; it's supposed to be delayed till the graphics synch point;
		///		1. Invokation arguments are: (<List of added descriptors>, <number of descriptors added>)
		/// </summary>
		virtual Event<const Reference<GraphicsObjectDescriptor>*, size_t>& OnSceneObjectsAdded() = 0;

		/// <summary> 
		/// Invoked, whenever the scene graphics objects get removed 
		/// Notes:
		///		0. RemoveSceneObject does not directly trigger this; it's supposed to be delayed till the graphics synch point;
		///		1. Invokation arguments are: (<List of removed descriptors>, <number of descriptors removed>)
		/// </summary>
		virtual Event<const Reference<GraphicsObjectDescriptor>*, size_t>& OnSceneObjectsRemoved() = 0;

		/// <summary>
		/// Gives access to all currently existing graphics scene objects
		/// </summary>
		/// <param name="descriptors"> List of objects </param>
		/// <param name="count"> Number of objects </param>
		virtual void GetSceneObjects(const Reference<GraphicsObjectDescriptor>*& descriptors, size_t& count) = 0;


		/// <summary>
		/// Translates light type name to unique type identifier that can be used within the shaders
		/// </summary>
		/// <param name="lightTypeName"> Light type name </param>
		/// <param name="lightTypeId"> Reference to store light type identifier at </param>
		/// <returns> True, if light type was found </returns>
		virtual bool GetLightTypeId(const std::string& lightTypeName, uint32_t& lightTypeId)const = 0;

		/// <summary> Number of bytes needed per  </summary>
		virtual size_t PerLightDataSize()const = 0;

		/// <summary>
		/// Schedules the light descriptor to be added to the graphics context for the next graphics synch point
		/// </summary>
		/// <param name="descriptor"> Descriptor to add </param>
		virtual void AddSceneLightDescriptor(LightDescriptor* descriptor) = 0;

		/// <summary>
		/// Schedules the light descriptor to be removed from the graphics context for the next graphics synch point
		/// </summary>
		/// <param name="descriptor"> Descriptor to remove </param>
		virtual void RemoveSceneLightDescriptor(LightDescriptor* descriptor) = 0;

		/// <summary> 
		/// Invoked, whenever the scene light descriptors get added 
		/// Notes:
		///		0. AddSceneLightDescriptor does not directly trigger this; it's supposed to be delayed till the graphics synch point;
		///		1. Invokation arguments are: (<List of added descriptors>, <number of descriptors added>)
		/// </summary>
		virtual Event<const Reference<LightDescriptor>*, size_t>& OnSceneLightDescriptorsAdded() = 0;

		/// <summary> 
		/// Invoked, whenever the scene light descriptors get removed 
		/// Notes:
		///		0. RemoveSceneLightDescriptor does not directly trigger this; it's supposed to be delayed till the graphics synch point;
		///		1. Invokation arguments are: (<List of removed descriptors>, <number of descriptors removed>)
		/// </summary>
		virtual Event<const Reference<LightDescriptor>*, size_t>& OnSceneLightDescriptorsRemoved() = 0;

		/// <summary>
		/// Gives access to all currently existing light descriptors
		/// </summary>
		/// <param name="pipelines"> List of light descriptors </param>
		/// <param name="count"> Number of light descriptors </param>
		virtual void GetSceneLightDescriptors(const Reference<LightDescriptor>*& descriptors, size_t& count) = 0;
	};
}
