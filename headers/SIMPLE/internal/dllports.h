#pragma once

#ifdef _WIN32
#ifdef SIMPLE_EXPORT
#define SIMPLE_DLL __declspec(dllexport)
#else
#define SIMPLE_DLL __declspec(dllimport)
#endif
#define PLUGIN_EXPORT __declspec(dllexport)
#else
#define SIMPLE_DLL
#ifdef ORYOL_EMSCRIPTEN
#include <emscripten.h>
#define PLUGIN_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define PLUGIN_EXPORT
#endif
#endif
