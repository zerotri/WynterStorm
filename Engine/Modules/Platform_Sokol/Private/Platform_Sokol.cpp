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

#include "sokol_common.h"

static ws_settings_t system_settings;
static sg_desc app_description;
static simgui_desc_t imgui_description;
static sg_imgui_t sg_imgui_description;
static sg_pass_action default_pass_action;

extern void start();
extern void end();
extern void load();
extern void unload();
extern void tick(float delta_time);
extern void render(float delta_time);

static void init(void) {
    
    app_description.context = sapp_sgcontext();
    sg_setup(&app_description);

    simgui_setup(&imgui_description);
    sg_imgui_init(&sg_imgui_description);

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
}

static void frame(void) {
    simgui_new_frame(sapp_width(), sapp_height(), 1.0f/60.0f);

    tick(0.0f);

    render(0.0f);

    sg_imgui_draw(&sg_imgui_description);

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Debugging")) {
            ImGui::MenuItem("Buffers", 0, &sg_imgui_description.buffers.open);
            ImGui::MenuItem("Images", 0, &sg_imgui_description.images.open);
            ImGui::MenuItem("Shaders", 0, &sg_imgui_description.shaders.open);
            ImGui::MenuItem("Pipelines", 0, &sg_imgui_description.pipelines.open);
            ImGui::MenuItem("Passes", 0, &sg_imgui_description.passes.open);
            ImGui::MenuItem("Calls", 0, &sg_imgui_description.capture.open);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    sg_begin_default_pass(&default_pass_action, sapp_width(), sapp_height());
    simgui_render();
    sg_end_pass();
    sg_commit();
}

static void cleanup(void) {
    sg_imgui_discard(&sg_imgui_description);
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

    default_pass_action.colors[0].action = SG_ACTION_CLEAR;
    default_pass_action.colors[0].val[0] = 1.0f;
    default_pass_action.colors[0].val[1] = 1.0f;
    default_pass_action.colors[0].val[2] = 1.0f;
    default_pass_action.colors[0].val[3] = 1.0f;

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
    default_pass_action.colors[0].action = SG_ACTION_CLEAR;
    default_pass_action.colors[0].val[0] = color.r;
    default_pass_action.colors[0].val[1] = color.g;
    default_pass_action.colors[0].val[2] = color.b;
    default_pass_action.colors[0].val[3] = color.a;
}