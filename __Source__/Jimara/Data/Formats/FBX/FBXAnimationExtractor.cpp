#include "FBXAnimationExtractor.h"


namespace Jimara {
	namespace FBXHelpers {
		namespace {
			// [2 - 8] (1 << [1 - 3])
			enum class InterpolationType : int64_t {
				eInterpolationConstant = 0x00000002,
				eInterpolationLinear = 0x00000004,
				eInterpolationCubic = 0x00000008
			};

			// [256 - 16384] (1 << [8 - 14])
			enum class TangentMode : int64_t {
				eTangentAuto = 0x00000100,
				eTangentTCB = 0x00000200,
				eTangentUser = 0x00000400,
				eTangentGenericBreak = 0x00000800,
				eTangentBreak = eTangentGenericBreak | eTangentUser,
				eTangentAutoBreak = eTangentGenericBreak | eTangentAuto,
				eTangentGenericClamp = 0x00001000,
				eTangentGenericTimeIndependent = 0x00002000,
				eTangentGenericClampProgressive = 0x00004000 | eTangentGenericTimeIndependent
			};

			// [16777216 - 33554432] (1 << [24 - 25])
			enum class WeightedMode : int64_t {
				eWeightedNone = 0x00000000,
				eWeightedRight = 0x01000000,
				eWeightedNextLeft = 0x02000000,
				eWeightedAll = eWeightedRight | eWeightedNextLeft
			};

			// [268435456 - 536870912] (1 << [28 - 29])
			enum class VelocityMode : int64_t {
				eVelocityNone = 0x00000000,
				eVelocityRight = 0x10000000,
				eVelocityNextLeft = 0x20000000,
				eVelocityAll = eVelocityRight | eVelocityNextLeft
			};

			// [256] (1 << 8)
			enum class ConstantMode : int64_t {
				eConstantStandard = 0x00000000,
				eConstantNext = 0x00000100
			};

			// [1048576 - 2097152] (1 << [20 - 21])
			enum class TangentVisibility : int64_t {
				eTangentShowNone = 0x00000000,
				eTangentShowLeft = 0x00100000,
				eTangentShowRight = 0x00200000,
				eTangentShowBoth = eTangentShowLeft | eTangentShowRight
			};
		}
	}
}
