#include "sokol_common.h"

struct graphics_state_t {
    sg_desc app_description;
    simgui_desc_t simgui_description;
    sg_imgui_t sg_imgui;
    sg_pipeline_desc editor_pipeline_description;
    sg_pipeline editor_pipeline;
    sg_pipeline_desc game_pipeline_description;
    sg_pipeline game_pipeline;
    sg_bindings bind;
    sg_pass game_pass;
    sg_pass_desc game_pass_description;
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