#pragma once

#pragma warning(disable: 4251)
#ifdef _WIN32
#ifdef JIMARA_DLL_EXPORTS
/// <summary> Classes/Functions marked with this will be exported in Jimara DLL </summary>
#define JIMARA_API __declspec(dllexport)
#else
/// <summary> Classes/Functions marked with this will be exported in Jimara DLL </summary>
#define JIMARA_API __declspec(dllimport)
#endif
#else
/// <summary> Classes/Functions marked with this will be exported in Jimara DLL </summary>
#define JIMARA_API
#endif
