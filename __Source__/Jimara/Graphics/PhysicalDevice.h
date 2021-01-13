#pragma once
#include "../Core/Object.h"
namespace Jimara { namespace Graphics { class PhysicalDevice; } }
#include "GraphicsInstance.h"
#include "GraphicsDevice.h"

namespace Jimara {
	namespace Graphics {
		/// <summary>
		/// Physical graphics device 
		/// (Can be an actual discrete/intergated GPU or CPU or even some virtual amalgamation of those and more... 
		/// All the user should care about is that this object has certain graphics capabilities)
		/// </summary>
		class PhysicalDevice {
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
			enum class DeviceFeature : uint64_t {
				/// <summary> Good old graphics capability </summary>
				GRAPHICS = (1 << 0),

				/// <summary> Arbitrary support for compute </summary>
				COMPUTE = (1 << 1),

				/// <summary> Synchronous graphics & compute </summary>
				SYNCHRONOUS_COMPUTE = (1 << 2),

				/// <summary> Asynchronous graphics & compute </summary>
				ASYNCHRONOUS_COMPUTE = (1 << 3),

				/// <summary> Swap chain support </summary>
				SWAP_CHAIN = (1 << 4),

				/// <summary> Unisotropic filtering support (needed for mipmaps) </summary>
				SAMPLER_ANISOTROPY = (1 << 5),

				/// <summary> All capabilities </summary>
				ALL = (~((uint64_t)0))
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
			virtual uint64_t Features()const = 0;

			/// <summary>
			/// Tells if Features() contains given feature
			/// </summary>
			/// <param name="feature"> Feature to check </param>
			/// <returns> True, if feature present </returns>
			bool HasFeature(DeviceFeature feature)const;

			/// <summary> Phisical device name/title </summary>
			virtual const char* Name()const = 0;

			/// <summary> Device VRAM(memory) capacity in bytes </summary>
			virtual size_t VramCapacity()const = 0;

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
	}
}
