#include <WynterStorm.h>

#include <string>
#include <map>
#include <vector>

// sokol
#ifdef __APPLE__
#elif defined(_WIN32)
#define SOKOL_IMPL
#define SOKOL_IMGUI_IMPL
#define SOKOL_GFX_IMGUI_IMPL
#elif defined(__EMSCRIPTEN__)
#define SOKOL_IMPL
#define SOKOL_IMGUI_IMPL
#define SOKOL_GFX_IMGUI_IMPL
#endif

#include "platform_sokol_internal.h"

#define HANDMADE_MATH_IMPLEMENTATION
#define HANDMADE_MATH_NO_SSE
#include <HandmadeMath.h>
#include "default.shader.h"


static ws_settings_t system_settings;
graphics_state_t graphics_state;
sg_shader shader_default;

extern void start();
extern void end();
extern void load();
extern void unload();
extern void tick(float delta_time);
extern void render(float delta_time);

std::vector<ws_vertex_t> vertex_buffer;
std::vector<uint16_t> index_buffer;

static void init(void) {
    
    graphics_state.app_description.context = sapp_sgcontext();
    sg_setup(&graphics_state.app_description);

    sg_imgui_init(&graphics_state.sg_imgui);
    simgui_setup(&graphics_state.simgui_description);

    shader_default = sg_make_shader(default_shader_desc());

    graphics_state.pipeline = sg_alloc_pipeline();
    auto& pipeline_description = graphics_state.pipeline_description;

    pipeline_description = {0};

    pipeline_description.shader = shader_default;
    pipeline_description.layout.buffers[0].stride = sizeof(ws_vertex_t);
    pipeline_description.layout.attrs[ATTR_vs_pos].format = SG_VERTEXFORMAT_FLOAT3;
    pipeline_description.layout.attrs[ATTR_vs_color0].format = SG_VERTEXFORMAT_FLOAT4;
    pipeline_description.layout.attrs[ATTR_vs_texcoord0].format = SG_VERTEXFORMAT_FLOAT2;
    pipeline_description.blend.enabled = true;
    pipeline_description.blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
    pipeline_description.blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    pipeline_description.blend.color_write_mask = SG_COLORMASK_RGB;
    pipeline_description.index_type = SG_INDEXTYPE_UINT16;
    pipeline_description.depth_stencil.depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL;
    pipeline_description.depth_stencil.depth_write_enabled = true;
    pipeline_description.rasterizer.cull_mode = SG_CULLMODE_NONE;
    pipeline_description.label = "default-2d";

    sg_init_pipeline(graphics_state.pipeline, &pipeline_description);

    ImGuiStyle* style = &ImGui::GetStyle();
	ImVec4* colors = style->Colors;

	colors[ImGuiCol_Text]                   = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
	colors[ImGuiCol_TextDisabled]           = ImVec4(0.500f, 0.500f, 0.500f, 1.000f);
	colors[ImGuiCol_WindowBg]               = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
	colors[ImGuiCol_ChildBg]                = ImVec4(0.280f, 0.280f, 0.280f, 0.000f);
	colors[ImGuiCol_PopupBg]                = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);
	colors[ImGuiCol_Border]                 = ImVec4(0.266f, 0.266f, 0.266f, 1.000f);
	colors[ImGuiCol_BorderShadow]           = ImVec4(0.000f, 0.000f, 0.000f, 0.200f);
	colors[ImGuiCol_FrameBg]                = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
	colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.200f, 0.200f, 0.200f, 1.000f);
	colors[ImGuiCol_FrameBgActive]          = ImVec4(0.280f, 0.280f, 0.280f, 1.000f);
	colors[ImGuiCol_TitleBg]                = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
	colors[ImGuiCol_TitleBgActive]          = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
	colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
	colors[ImGuiCol_MenuBarBg]              = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
	colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
	colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.277f, 0.277f, 0.277f, 1.000f);
	colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.300f, 0.300f, 0.300f, 1.000f);
	colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_CheckMark]              = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
	colors[ImGuiCol_SliderGrab]             = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
	colors[ImGuiCol_SliderGrabActive]       = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_Button]                 = ImVec4(1.000f, 1.000f, 1.000f, 0.000f);
	colors[ImGuiCol_ButtonHovered]          = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
	colors[ImGuiCol_ButtonActive]           = ImVec4(1.000f, 1.000f, 1.000f, 0.391f);
	colors[ImGuiCol_Header]                 = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);
	colors[ImGuiCol_HeaderHovered]          = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
	colors[ImGuiCol_HeaderActive]           = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
	colors[ImGuiCol_Separator]              = colors[ImGuiCol_Border];
	colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
	colors[ImGuiCol_SeparatorActive]        = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_ResizeGrip]             = ImVec4(1.000f, 1.000f, 1.000f, 0.250f);
	colors[ImGuiCol_ResizeGripHovered]      = ImVec4(1.000f, 1.000f, 1.000f, 0.670f);
	colors[ImGuiCol_ResizeGripActive]       = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_Tab]                    = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
	colors[ImGuiCol_TabHovered]             = ImVec4(0.352f, 0.352f, 0.352f, 1.000f);
	colors[ImGuiCol_TabActive]              = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
	colors[ImGuiCol_TabUnfocused]           = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
	colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
	// colors[ImGuiCol_DockingPreview]         = ImVec4(1.000f, 0.391f, 0.000f, 0.781f);
	// colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
	colors[ImGuiCol_PlotLines]              = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
	colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_PlotHistogram]          = ImVec4(0.586f, 0.586f, 0.586f, 1.000f);
	colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_TextSelectedBg]         = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
	colors[ImGuiCol_DragDropTarget]         = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_NavHighlight]           = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);
	colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);

	style->ChildRounding = 4.0f;
	style->FrameBorderSize = 1.0f;
	style->FrameRounding = 2.0f;
	style->GrabMinSize = 7.0f;
	style->PopupRounding = 2.0f;
	style->ScrollbarRounding = 12.0f;
	style->ScrollbarSize = 13.0f;
	style->TabBorderSize = 1.0f;
	style->TabRounding = 0.0f;
	style->WindowRounding = 4.0f;

    load();
}

static void frame(void) {
    sfetch_dowork();


    tick(0.0f);
    
    simgui_new_frame(sapp_width(), sapp_height(), 1.0f/60.0f);


    ws_sprite_batcher_reset();
    sg_begin_default_pass(&graphics_state.pass_action, sapp_width(), sapp_height());

    render(0.0f);

    sg_push_debug_group("sprite-batcher");
    ws_sprite_batcher_finish();
    sg_pop_debug_group();

    sg_imgui_draw(&graphics_state.sg_imgui);

    auto &sg_imgui = graphics_state.sg_imgui;
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Debugging")) {
            ImGui::MenuItem("Buffers", 0, &sg_imgui.buffers.open);
            ImGui::MenuItem("Images", 0, &sg_imgui.images.open);
            ImGui::MenuItem("Shaders", 0, &sg_imgui.shaders.open);
            ImGui::MenuItem("Pipelines", 0, &sg_imgui.pipelines.open);
            ImGui::MenuItem("Passes", 0, &sg_imgui.passes.open);
            ImGui::MenuItem("Calls", 0, &sg_imgui.capture.open);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    simgui_render();
    sg_end_pass();
    sg_commit();
}

static void cleanup(void) {
    sg_imgui_discard(&graphics_state.sg_imgui);
    simgui_shutdown();
    sg_shutdown();
}

static void event(const sapp_event* event) {
    simgui_handle_event(event);
}
sapp_desc sokol_main(int argc, char* argv[]) {

    system_settings.screen.width = 320;
    system_settings.screen.height = 240;
    system_settings.screen.hidpi = false;
    system_settings.screen.vsync = true;

    stm_setup();
    sfetch_desc_t fetch_desc{ 0 };
    sfetch_setup(&fetch_desc);

    auto &pass_action = graphics_state.pass_action;
    pass_action.colors[0].action = SG_ACTION_CLEAR;
    pass_action.colors[0].val[0] = 1.0f;
    pass_action.colors[0].val[1] = 1.0f;
    pass_action.colors[0].val[2] = 1.0f;
    pass_action.colors[0].val[3] = 1.0f;

    start();

    sapp_desc description = {0};
    description.width = system_settings.screen.width;
    description.height = system_settings.screen.height;
    description.high_dpi = system_settings.screen.hidpi;
    description.window_title = system_settings.screen.title;
    description.init_cb = init;
    description.frame_cb = frame;
    description.cleanup_cb = cleanup;
    description.event_cb = event;
    return description;
}

ws_settings_t* ws_system_settings()
{
    return &system_settings;
}

float ws_time_current()
{
    return (float)stm_sec(stm_now());
}

void ws_display_set_clear_color( ws_color_t color )
{
    auto &pass_action = graphics_state.pass_action;
    pass_action.colors[0].action = SG_ACTION_CLEAR;
    pass_action.colors[0].val[0] = color.r;
    pass_action.colors[0].val[1] = color.g;
    pass_action.colors[0].val[2] = color.b;
    pass_action.colors[0].val[3] = color.a;
}