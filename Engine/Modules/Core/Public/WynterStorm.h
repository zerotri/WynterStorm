#pragma once
#include <cstdint>

#include <WynterStorm/Coroutine.h>

struct ws_handle_t { uintptr_t address; };

union ws_color_t {
    struct {
        float r;
        float g;
        float b;
        float a;
    };
    float color[4];
};

struct ws_vec2d_t {
    float x;
    float y;
};

struct ws_settings_t {
    struct {
        int width;
        int height;
        bool hidpi;
        bool vsync;
        const char *title;
    } screen;
};

// colors

extern ws_color_t ws_color_pink;

ws_handle_t ws_sprite_load( const char *file_path );

void ws_display_set_clear_color( ws_color_t color );
ws_settings_t* ws_system_settings();
float ws_time_current();