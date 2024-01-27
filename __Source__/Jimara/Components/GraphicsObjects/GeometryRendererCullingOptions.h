#pragma once
#include "../../Data/ConfigurableResource.h"
#include "../../Core/Property.h"


namespace Jimara {
	/// <summary> Register ConfigurableResource </summary>
	JIMARA_REGISTER_TYPE(Jimara::GeometryRendererCullingOptions::Provider);



	/// <summary> Renderer cull options </summary>
	struct JIMARA_API GeometryRendererCullingOptions {
		/// <summary> 
		/// 'Natural' culling boundary of the geometry will be expanded by this amount in each direction in local space 
		/// (Useful for the cases when the shader does some vertex displacement and the visible geometry goes out of initial boundaries)
		/// </summary>
		Vector3 boundaryThickness = Vector3(0.0f);

		/// <summary> Local-space culling boundary will be offset by this amount </summary>
		Vector3 boundaryOffset = Vector3(0.0f);

		/// <summary> 
		/// Object will be visible if and only if the object occupies a fraction of the viewport 
		/// between onScreenSizeRangeStart and onScreenSizeRangeEnd 
		/// <para/> If onScreenSizeRangeEnd is less than 0, maximal on-screen size will be considered infinite
		/// </summary>
		float onScreenSizeRangeStart = 0.0f;

		/// <summary> 
		/// Object will be visible if and only if the object occupies a fraction of the viewport 
		/// between onScreenSizeRangeStart and onScreenSizeRangeEnd 
		/// <para/> If onScreenSizeRangeEnd is less than 0, maximal on-screen size will be considered infinite
		/// </summary>
		float onScreenSizeRangeEnd = -1.0f;

		/// <summary>
		/// Comparizon operator
		/// </summary>
		/// <param name="other"> Other options </param>
		/// <returns> True, if the options are the same </returns>
		bool operator==(const GeometryRendererCullingOptions& other)const;

		/// <summary>
		/// Comparizon operator
		/// </summary>
		/// <param name="other"> Other options </param>
		/// <returns> True, if the options are not the same </returns>
		inline bool operator!=(const GeometryRendererCullingOptions& other)const { return !((*this) == other); }

		/// <summary> Default serializer of CullingOptions </summary>
		struct JIMARA_API Serializer;

		/// <summary> Provider resource for GeometryRendererCullingOptions </summary>
		class JIMARA_API Provider;

		/// <summary> Simple wrapper with existing midifiable configuration and the listener </summary>
		class JIMARA_API ConfigurableOptions;
	};



	/// <summary> Default serializer of CullingOptions </summary>
	struct JIMARA_API GeometryRendererCullingOptions::Serializer : public virtual Serialization::SerializerList::From<GeometryRendererCullingOptions> {
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="name"> Name of the ItemSerializer </param>
		/// <param name="hint"> Target hint (editor helper texts on hover and what not) </param>
		/// <param name="attributes"> Serializer attributes </param>
		inline Serializer(const std::string_view& name, const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {})
			: Serialization::ItemSerializer(name, hint, attributes) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~Serializer() {}

		/// <summary>
		/// Gives access to sub-serializers/fields
		/// </summary>
		/// <param name="recordElement"> Each sub-serializer will be reported by invoking this callback with serializer & corresonding target as parameters </param>
		/// <param name="targetAddr"> Serializer target object </param>
		virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, GeometryRendererCullingOptions* target)const override;
	};



	/// <summary> Provider resource for GeometryRendererCullingOptions </summary>
	class JIMARA_API GeometryRendererCullingOptions::Provider : public virtual ConfigurableResource {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="options"> Culling options </param>
		inline Provider(const GeometryRendererCullingOptions& options) : m_options(options) {}

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name=""> Ignored </param>
		inline Provider(const ConfigurableResource::CreateArgs& = {}) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~Provider() { }

		/// <summary> Culling options </summary>
		GeometryRendererCullingOptions Options()const;

		/// <summary> Culling options </summary>
		inline Property<GeometryRendererCullingOptions> Options() {
			typedef GeometryRendererCullingOptions(*GetFn)(const Provider*);
			static const GetFn get = [](const Provider* self) { return self->Options(); };
			return Property<GeometryRendererCullingOptions>(Function(get, (const Provider*)this), Callback(&Provider::SetOptions, this));
		}

		/// <summary>
		/// Changes culling options
		/// </summary>
		/// <param name="options"> Options to use </param>
		void SetOptions(const GeometryRendererCullingOptions& options);

		/// <summary> Event, invoked each time the Options are altered </summary>
		inline Event<const GeometryRendererCullingOptions::Provider*>& OnOptionsChanged()const { return m_onDirty; }

		/// <summary>
		/// Gives access to sub-serializers/fields
		/// </summary>
		/// <param name="recordElement"> Each sub-serializer will be reported by invoking this callback with serializer & corresonding target as parameters </param>
		virtual void GetFields(const Callback<Serialization::SerializedObject> recordElement) override;

	private:
		mutable SpinLock m_lock;
		GeometryRendererCullingOptions m_options;
		mutable EventInstance<const GeometryRendererCullingOptions::Provider*> m_onDirty;
	};

	/// <summary> Simple wrapper with existing midifiable configuration and the listener </summary>
	class JIMARA_API GeometryRendererCullingOptions::ConfigurableOptions final {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="configuration"> Initial configuration </param>
		inline ConfigurableOptions(const GeometryRendererCullingOptions& configuration = {}) : m_configuration(configuration) {}

		/// <summary>
		/// Copy-constructor
		/// </summary>
		/// <param name="other"> Options to copy </param>
		ConfigurableOptions(const ConfigurableOptions& other);

		/// <summary> Destructor </summary>
		~ConfigurableOptions();

		/// <summary>
		/// Copy-assignment
		/// </summary>
		/// <param name="other"> Options to copy </param>
		/// <returns> Self </returns>
		ConfigurableOptions& operator=(const ConfigurableOptions& other);

		/// <summary> Type-cast to options </summary>
		inline operator const GeometryRendererCullingOptions& ()const { return m_configuration; }

		/// <summary>
		/// Assigns custom ConfigurableOptions (this may decouple active configuration value from the provider)
		/// </summary>
		/// <param name="options"> Options to use </param>
		/// <returns> Self </returns>
		ConfigurableOptions& operator=(const GeometryRendererCullingOptions& options);

		/// <summary> Event, invoked, each time the underlying ConfigurableOptions change </summary>
		Event<const ConfigurableOptions*>& OnDirty()const { return m_onDirty; }

		/// <summary> Provider resource for GeometryRendererCullingOptions </summary>
		Reference<Provider> ConfigurationProvider()const;

		/// <summary>
		/// Sets Provider for GeometryRendererCullingOptions
		/// </summary>
		/// <param name="provider"> GeometryRendererCullingOptions::Provider </param>
		void SetConfigurationProvider(Provider* provider);

		/// <summary>
		/// Serializer for ConfigurableOptions
		/// </summary>
		struct JIMARA_API Serializer : public virtual Serialization::SerializerList::From<ConfigurableOptions> {
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="name"> Name of the ItemSerializer </param>
			/// <param name="hint"> Target hint (editor helper texts on hover and what not) </param>
			/// <param name="attributes"> Serializer attributes </param>
			inline Serializer(const std::string_view& name, const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {})
				: Serialization::ItemSerializer(name, hint, attributes) {}

			/// <summary> Virtual destructor </summary>
			inline virtual ~Serializer() {}

			/// <summary>
			/// Gives access to sub-serializers/fields
			/// </summary>
			/// <param name="recordElement"> Each sub-serializer will be reported by invoking this callback with serializer & corresonding target as parameters </param>
			/// <param name="targetAddr"> Serializer target object </param>
			virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, ConfigurableOptions* target)const override;
		};

	private:
		GeometryRendererCullingOptions m_configuration;
		mutable SpinLock m_providerLock;
		Reference<Provider> m_configurationProvider;
		mutable EventInstance<const ConfigurableOptions*> m_onDirty;
	};



	// Type details
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<GeometryRendererCullingOptions::Provider>(const Callback<const Object*>& report);
	template<> inline void TypeIdDetails::GetParentTypesOf<GeometryRendererCullingOptions::Provider>(const Callback<TypeId>& report) {
		report(TypeId::Of<ConfigurableResource>());
	}
}
