#pragma once
#ifdef __APPLE__
#include "../../Core/Function.h"
#include "../../Core/JimaraApi.h"


namespace Jimara {
	namespace OS {
		namespace MainThreadCallbacks {

			int JIMARA_API RunOnSecondaryThread(const Function<int, int, char**>& process, int argc, char** argv);

			template<typename CallType>
			inline int RunOnSecondaryThread(const CallType& process, int argc, char** argv) {
				return RunOnSecondaryThread(Function<int, int, char**>::FromCall(&process), argc, argv);
			}

			void JIMARA_API ExecuteOnMainThread(const Callback<>& action);

			template<typename CallType>
			inline void ExecuteOnMainThread(const CallType& action) {
				ExecuteOnMainThread(Callback<>::FromCall(&action));
			}
		}
	}
}
#endif
