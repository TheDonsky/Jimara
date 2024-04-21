#pragma once
#include "../Core/Object.h"
#include "../Core/EnumClassBooleanOperands.h"
namespace Jimara { namespace Graphics { class PhysicalDevice; } }
#include "GraphicsInstance.h"
#include "GraphicsDevice.h"
#include "Memory/Texture.h"


namespace Jimara {
	namespace Graphics {
		/// <summary>
		/// Physical graphics device 
		/// (Can be an actual discrete/intergated GPU or CPU or even some virtual amalgamation of those and more... 
		/// All the user should care about is that this object has certain graphics capabilities)
		/// </summary>
		class JIMARA_API PhysicalDevice {
		public:
			/// <summary> Physical device type </summary>
			enum class DeviceType : uint8_t {
				/// <summary> Unknown </summary>
				OTHER = 0,

				/// <summary> Central processing unit </summary>
				CPU = 1,

				/// <summary> Integrated graphics processor </summary>
				INTEGRATED = 2,

				/// <summary> Discrete graphics processing unit </summary>
				DESCRETE = 3,

				/// <summary> Some kind of a virtual graphics device </summary>
				VIRTUAL = 4
			};

			/// <summary> Physical device features (bitmask) </summary>
			enum class DeviceFeatures : uint64_t {
				/// <summary> Empty bitmask </summary>
				NONE = 0u,

				/// <summary> Good old graphics capability </summary>
				GRAPHICS = (1u << 0u),

				/// <summary> Arbitrary support for compute </summary>
				COMPUTE = (1u << 1u),

				/// <summary> Synchronous graphics & compute </summary>
				SYNCHRONOUS_COMPUTE = (1u << 2u),

				/// <summary> Asynchronous graphics & compute </summary>
				ASYNCHRONOUS_COMPUTE = (1u << 3u),

				/// <summary> Swap chain support </summary>
				SWAP_CHAIN = (1u << 4u),

				/// <summary> Unisotropic filtering support (needed for mipmaps) </summary>
				SAMPLER_ANISOTROPY = (1u << 5u),

				/// <summary> Support for GL_ARB_fragment_shader_interlock </summary>
				FRAGMENT_SHADER_INTERLOCK = (1u << 6u),

				/// <summary> Support for Ray-Tracing features </summary>
				RAY_TRACING = (1u << 7u),

				/// <summary> All capabilities </summary>
				ALL = (~((uint64_t)0u))
			};



			/// <summary> Virtual destructor </summary>
			virtual ~PhysicalDevice();

			/// <summary> Increments reference count </summary>
			virtual void AddRef()const;

			/// <summary> Decrements reference count </summary>
			virtual void ReleaseRef()const;

			/// <summary> "Owner" Graphics instance </summary>
			Graphics::GraphicsInstance* GraphicsInstance()const;

			/// <summary> Logger </summary>
			OS::Logger* Log()const;



			/// <summary> Physical device type </summary>
			virtual DeviceType Type()const = 0;

			/// <summary> Physical device features </summary>
			virtual DeviceFeatures Features()const = 0;

			/// <summary>
			/// Tells if Features() contains given feature
			/// </summary>
			/// <param name="features"> Feature set to check </param>
			/// <returns> True, if all features are present </returns>
			inline bool HasFeatures(DeviceFeatures features)const;

			/// <summary> Phisical device name/title </summary>
			virtual const char* Name()const = 0;

			/// <summary> Device VRAM(memory) capacity in bytes </summary>
			virtual size_t VramCapacity()const = 0;

			/// <summary> Maximal available Multisampling this device is capable of </summary>
			virtual Texture::Multisampling MaxMultisapling()const = 0;

			/// <summary> Instantiates a logical device </summary>
			virtual Reference<GraphicsDevice> CreateLogicalDevice() = 0;

		protected:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="instance"> Owner </param>
			PhysicalDevice(Graphics::GraphicsInstance* instance);

		private:
			// 'Owner' instance
			Graphics::GraphicsInstance* m_owner;

			// Reference counter should not be able to be a reson of some random fuck-ups, so let us disable copy/move-constructor/assignments:
			PhysicalDevice(const PhysicalDevice&) = delete;
			PhysicalDevice& operator=(const PhysicalDevice&) = delete;
			PhysicalDevice(PhysicalDevice&&) = delete;
			PhysicalDevice& operator=(PhysicalDevice&&) = delete;
		};


		JIMARA_DEFINE_ENUMERATION_BOOLEAN_OPERATIONS(PhysicalDevice::DeviceFeatures);

		/// <summary>
		/// Prints out feature bitmask as a list of corresponding strings
		/// </summary>
		/// <param name="stream"> Output stream </param>
		/// <param name="features"> Feature mask </param>
		/// <returns> stream </returns>
		JIMARA_API std::ostream& operator<<(std::ostream& stream, PhysicalDevice::DeviceFeatures features);
	}
}
