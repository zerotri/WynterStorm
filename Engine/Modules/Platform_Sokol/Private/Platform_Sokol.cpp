#include <WynterStorm.h>

#include <string>
#include <map>
#include <vector>

// sokol
#ifdef __APPLE__
#elif defined(_WIN32)
#define SOKOL_IMPL
#define SOKOL_D3D11
#elif defined(__EMSCRIPTEN__)
#define SOKOL_IMPL
#define SOKOL_GLES2
#endif

#include <sokol_app.h>
#include <sokol_gfx.h>
#include <sokol_glue.h>

sg_pass_action pass_action;
sg_desc app_description;
void init(void) {
    
    app_description.context = sapp_sgcontext();
    sg_setup(&app_description);

    sg_color_attachment_action color = { SG_ACTION_CLEAR, {1.0f, 0.0f, 0.0f, 1.0f } };
    pass_action.colors[0] = color;
}

void frame(void) {
    float g = pass_action.colors[0].val[1] + 0.01f;
    pass_action.colors[0].val[1] = (g > 1.0f) ? 0.0f : g;
    sg_begin_default_pass(&pass_action, sapp_width(), sapp_height());
    sg_end_pass();
    sg_commit();
}

void cleanup(void) {
    sg_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[]) {
    sapp_desc description = {0};
    description.width = 640;
    description.height = 480;
    description.init_cb = init;
    description.frame_cb = frame;
    description.cleanup_cb = cleanup;
    description.window_title = "WynterStorm バナナ";
    return description;
}