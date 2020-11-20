#pragma once
#ifdef __APPLE__
#define SOKOL_METAL
#elif defined(_WIN32)
#define SOKOL_D3D11
#elif defined(__EMSCRIPTEN__)
#define SOKOL_GLES2
#endif

#define SOKOL_TRACE_HOOKS
#define SFETCH_MAX_USERDATA_UINT64 32

#include <imgui.h>
#include <sokol_app.h>
#include <sokol_gfx.h>
#include <sokol_time.h>
#include <sokol_glue.h>
#include <sokol_fetch.h>
#include <util/sokol_imgui.h>
#include <util/sokol_gfx_imgui.h>