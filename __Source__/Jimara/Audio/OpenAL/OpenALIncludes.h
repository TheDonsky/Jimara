#pragma once

#if defined(__has_include)
#if __has_include(<AL/al.h>)
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#elif __has_include(<OpenAL/al.h>)
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#endif
#else
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#endif
