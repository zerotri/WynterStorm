#include "sokol_common.h"

struct graphics_state_t {
    sg_desc app_description;
    simgui_desc_t simgui_description;
    sg_imgui_t sg_imgui;
    sg_pipeline_desc pipeline_description;
    sg_pipeline pipeline;
    sg_bindings bind;
    sg_pass_action pass_action;
};

struct ws_vertex_t {
    float x, y, z;
    float r, g, b, a;
    float u, v;
};

extern graphics_state_t graphics_state;

void ws_sprite_batcher_reset();
void ws_sprite_batcher_finish();