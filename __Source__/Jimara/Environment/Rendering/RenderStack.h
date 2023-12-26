#pragma once
#include "RenderImages.h"


namespace Jimara {
	/// <summary>
	/// Glorified list of sequential renderers, generating the main or nor so main Rendered output
	/// </summary>
	class JIMARA_API RenderStack : public virtual Object {
	public:
		/// <summary>
		/// Main render stack for the scene (this one will be used to render the image that will be displayed to screen)
		/// </summary>
		/// <param name="context"> Scene context </param>
		/// <returns> Main RenderStack for the scene </returns>
		static Reference<RenderStack> Main(Scene::LogicContext* context);

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="context"> Scene context </param>
		/// <param name="initialResolution"> Initial resolution to use </param>
		/// <param name="initialSampleCount"> Initial MSAA </param>
		RenderStack(Scene::LogicContext* context, 
			Size2 initialResolution = Size2(0u),
			Graphics::Texture::Multisampling initialSampleCount = Graphics::Texture::Multisampling::SAMPLE_COUNT_2);

		/// <summary> Virtual destructor </summary>
		virtual ~RenderStack();

		/// <summary> RenderStack image resolution </summary>
		Size2 Resolution()const;

		/// <summary>
		/// Sets RenderStack image resolution
		/// <para/> Notes:
		///		<para/> 0. The change will be applied and Images will be updated on the next frame;
		///		<para/> 1. Resolution() will be updated immediately, but Images() will stay the same till the next graphics synch point.
		/// </summary>
		/// <param name="resolution"> Resolution to use </param>
		void SetResolution(Size2 resolution);

		/// <summary> Multisampling (MSAA) </summary>
		Graphics::Texture::Multisampling SampleCount()const;

		/// <summary>
		/// Sets multisampling
		/// </summary>
		/// <param name="sampleCount"> Sample count to use (if the device does not support this many samples, the number will be truncated to what's available) </param>
		void SetSampleCount(Graphics::Texture::Multisampling sampleCount);

		/// <summary> Render image collection </summary>
		Reference<RenderImages> Images()const;

		/// <summary>
		/// Abstract renderer for final image generation
		/// <para/> Note: These renderers normally run as a part of the renderer stack in a well-defined order
		/// </summary>
		class Renderer;

		/// <summary>
		/// Adds a renderer to the stack
		/// <para/> Note: This takes effect after the graphics synch point.
		/// </summary>
		/// <param name="renderer"> Remderer to add </param>
		void AddRenderer(Renderer* renderer);

		/// <summary>
		/// Removes a renderer from the stack
		/// <para/> Note: This takes effect after the graphics synch point.
		/// </summary>
		/// <param name="renderer"> Remderer to remove </param>
		void RemoveRenderer(Renderer* renderer);

	private:
		// Underlying data
		struct Data;
		const Reference<Object> m_data;
	};

	/// <summary>
	/// Abstract renderer for final image generation
	/// <para/> Note: These renderers normally run as a part of the renderer stack in a well-defined order
	/// </summary>
	class JIMARA_API RenderStack::Renderer : public virtual Object {
	public:
		/// <summary>
		/// Should render the image to a bunch of images from given RenderImages collection
		/// <para/> Notes: 
		///		<para/> 0. Image collection will change if and only if the resolution and/or sample count of the render stack gets altered.
		///		<para/> 1. RenderStack executes Renderers one after the another, passing the 'results' based on the category and the priority; 
		///		this means that not all renderers should be clearing the screen (overlays and postFX should definately do no such thing, for example).
		/// </summary>
		/// <param name="commandBufferInfo"> Command buffer and in-flight buffer index </param>
		/// <param name="images"> Set of textures to be used for rendering (set will change if resolution and/or multisampling changes) </param>
		virtual void Render(Graphics::InFlightBufferInfo commandBufferInfo, RenderImages* images) = 0;

		/// <summary>
		/// RenderStack gets executed as a Job in RenderJobs system; if any of the renderers that are part of it jave some jobs they depend on,
		/// they can report those through this callback
		/// </summary>
		/// <param name="report"> Dependencies can be reported through this callback </param>
		inline virtual void GetDependencies(Callback<JobSystem::Job*> report) { Unused(report); }

		/// <summary>
		/// Renderer 'category'
		/// <para/> Notes:
		///		<para/> 0. Lower category renderers will be executed first, followed by the higher category ones;
		///		<para/> 1. Global user interface may expose categories by something like an enumeration, 
		///		containing 'Camera/Geometry', 'PostFX', 'UI/Overlay' and such, but the engine internals do not care about any such thing;
		///		<para/> 2. If the categories match, higher Priority renderers will be called first;
		///		<para/> 3. Priorities are just numbers both in code and from UI;
		///		4. If both the category and priority are the same, rendering order is undefined
		/// </summary>
		inline uint32_t Category()const { return m_category.load(); }

		/// <summary>
		/// Sets the renderer category
		/// <para/> Note: Render job system will aknoweledge the change only after the graphics synch point
		/// </summary>
		/// <param name="category"> New category to assign this renderer to </param>
		inline void SetCategory(uint32_t category) { m_category = category; }

		/// <summary>
		/// Renderer 'priority' inside the same category
		/// <para/> Notes:
		///		<para/> 0. Lower category renderers will be executed first, followed by the higher category ones;
		///		<para/> 1. Global user interface may expose categories by something like an enumeration, 
		///		containing 'Camera/Geometry', 'PostFX', 'UI/Overlay' and such, but the engine internals do not care about any such thing;
		///		<para/> 2. If the categories match, higher Priority renderers will be called first;
		///		<para/> 3. Priorities are just numbers both in code and from UI;
		///		<para/> 4. If both the category and priority are the same, rendering order is undefined
		/// </summary>
		inline uint32_t Priority()const { return m_priority.load(); }

		/// <summary>
		/// Sets the renderer priority inside the same category
		/// Note: Render job system will aknoweledge the change only after the graphics synch point
		/// </summary>
		/// <param name="priority">  New priority to assign this renderer to </param>
		inline void SetPriority(uint32_t priority) { m_priority = priority; }

	private:
		// Category
		std::atomic<uint32_t> m_category = 0;

		// Priority
		std::atomic<uint32_t> m_priority = 0;
	};
}
