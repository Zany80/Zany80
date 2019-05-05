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
#define PLUGIN_EXPORT
#endif