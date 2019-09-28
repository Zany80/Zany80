#pragma once

#ifdef _WIN32
#ifdef ZANY_EXPORT
#define ZANY_DLL __declspec(dllexport)
#else
#define ZANY_DLL __declspec(dllimport)
#endif
#define PLUGIN_EXPORT __declspec(dllexport)
#else
#define ZANY_DLL
#ifdef ORYOL_EMSCRIPTEN
#include <emscripten.h>
#define PLUGIN_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define PLUGIN_EXPORT
#endif
#endif
