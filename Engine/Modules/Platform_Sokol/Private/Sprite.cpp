#include <WynterStorm.h>

// sokol
#include "sokol_common.h"

std::map<int16_t, struct ws_internal_sprite_t> sprites;

#define CUTE_ASEPRITE_IMPLEMENTATION
// #include <cute_aseprite.h>

ws_handle_t ws_sprite_load( const char *file_path )
{
    ws_handle_t emptyHandle;
    return emptyHandle;
}