#include "sokol_common.h"

struct ws_pipelines {
    enum {
        editor = 0,
        game = 1,
        max
    };
};

struct ws_shaders {
    enum {
        standard = 0,
        offscreen = 1,
        max
    };
};

struct ws_render_buffers {
    enum {
        frame = 0,
        depth = 1,
        max
    };
};

struct graphics_state_t {
    sg_imgui_t sg_imgui;
    sg_pipeline pipelines[ws_pipelines::max];
    sg_shader shaders[ws_shaders::max];
    sg_image render_buffers[ws_render_buffers::max];
    sg_bindings bind;
    sg_pass game_pass;
    sg_pass_action game_pass_action;
    sg_pass_action default_pass;
    int game_view_width;
    int game_view_height;
    int render_target_width;
    int render_target_height;
    bool render_target_valid;
};

struct ws_vertex_t {
    float x, y, z;
    float r, g, b, a;
    float u, v;
};

extern graphics_state_t graphics_state;

void ws_sprite_batcher_reset();
void ws_sprite_batcher_finish();