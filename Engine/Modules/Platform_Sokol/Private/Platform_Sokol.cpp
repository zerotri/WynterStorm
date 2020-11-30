#include <WynterStorm.h>
#include <TaggedHeap.h>

#include <inttypes.h>
#include <string>
#include <map>
#include <vector>

// sokol
#ifdef __APPLE__
#elif defined(_WIN32)
#define SOKOL_IMPL
#define SOKOL_IMGUI_IMPL
#define SOKOL_GFX_IMGUI_IMPL
#elif defined (__linux__)
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
sg_shader shader_offscreen;
sg_image color_img;

extern void start();
extern void end();
extern void load();
extern void unload();
extern void tick(double delta_time);
extern void render(double delta_time);

std::vector<ws_vertex_t> vertex_buffer;
std::vector<uint16_t> index_buffer;

static double last_tick_time = 0.0;
static double last_render_time = 0.0;

static void init(void) {
    
    // init memory for tagged heap allocator
    if(ws_tagged_heap_init() < 0)
    {
        // todo(Wynter): handle case where we can't init allocator memory
    }

    printf("Initialized tagged heap allocator\n");
    printf("  address: %" PRIxPTR "\n", (uintptr_t) ws_tagged_heap_get_base());
    printf("  block size: %" PRIxPTR "\n", (uintptr_t) ws_tagged_heap_get_block_size());

    graphics_state.app_description.context = sapp_sgcontext();
    sg_setup(&graphics_state.app_description);

    simgui_setup(&graphics_state.simgui_description);
    sg_imgui_init(&graphics_state.sg_imgui);

    shader_default = sg_make_shader(default_shader_desc());
    shader_offscreen = sg_make_shader(default_shader_desc());

    sg_image_desc img_desc = {0};
    img_desc.render_target = true;
    img_desc.width = 512;
    img_desc.height = 512;
    img_desc.pixel_format = SG_PIXELFORMAT_RGBA8;
    img_desc.min_filter = SG_FILTER_LINEAR;
    img_desc.mag_filter = SG_FILTER_LINEAR;
    img_desc.wrap_u = SG_WRAP_REPEAT;
    img_desc.wrap_v = SG_WRAP_REPEAT;
    img_desc.sample_count = 0;
    img_desc.label = "color-image";
    
    color_img = sg_make_image(&img_desc);

    img_desc.pixel_format = SG_PIXELFORMAT_DEPTH;
    img_desc.label = "depth-image";

    sg_image depth_img = sg_make_image(&img_desc);


    sg_pass_desc game_pass_description = {0};
    game_pass_description.color_attachments[0].image = color_img;
    game_pass_description.depth_stencil_attachment.image = depth_img;
    game_pass_description.label = "game-pass";

    graphics_state.game_pass = sg_make_pass(&game_pass_description);

    graphics_state.game_pipeline = sg_alloc_pipeline();
    auto& game_pipeline_description = graphics_state.game_pipeline_description;

    game_pipeline_description = {0};
    game_pipeline_description.shader = shader_offscreen;
    game_pipeline_description.layout.buffers[0].stride = sizeof(ws_vertex_t);
    game_pipeline_description.layout.attrs[ATTR_vs_pos].format = SG_VERTEXFORMAT_FLOAT3;
    game_pipeline_description.layout.attrs[ATTR_vs_color0].format = SG_VERTEXFORMAT_FLOAT4;
    game_pipeline_description.layout.attrs[ATTR_vs_texcoord0].format = SG_VERTEXFORMAT_FLOAT2;
    game_pipeline_description.blend.enabled = true;
    game_pipeline_description.blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
    game_pipeline_description.blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    game_pipeline_description.blend.color_write_mask = SG_COLORMASK_RGB;
    game_pipeline_description.blend.color_format = SG_PIXELFORMAT_RGBA8;
    game_pipeline_description.blend.depth_format = SG_PIXELFORMAT_DEPTH;
    game_pipeline_description.index_type = SG_INDEXTYPE_UINT16;
    game_pipeline_description.depth_stencil.depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL;
    game_pipeline_description.depth_stencil.depth_write_enabled = true;
    game_pipeline_description.rasterizer.cull_mode = SG_CULLMODE_NONE;
    game_pipeline_description.label = "game";

    sg_init_pipeline(graphics_state.game_pipeline, game_pipeline_description);

    graphics_state.editor_pipeline = sg_alloc_pipeline();
    auto& editor_pipeline_description = graphics_state.editor_pipeline_description;

    editor_pipeline_description = {0};
    editor_pipeline_description.shader = shader_default;
    editor_pipeline_description.layout.buffers[0].stride = sizeof(ws_vertex_t);
    editor_pipeline_description.layout.attrs[ATTR_vs_pos].format = SG_VERTEXFORMAT_FLOAT3;
    editor_pipeline_description.layout.attrs[ATTR_vs_color0].format = SG_VERTEXFORMAT_FLOAT4;
    editor_pipeline_description.layout.attrs[ATTR_vs_texcoord0].format = SG_VERTEXFORMAT_FLOAT2;
    editor_pipeline_description.blend.enabled = true;
    editor_pipeline_description.blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
    editor_pipeline_description.blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    editor_pipeline_description.blend.color_write_mask = SG_COLORMASK_RGB;
    editor_pipeline_description.index_type = SG_INDEXTYPE_UINT16;
    editor_pipeline_description.depth_stencil.depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL;
    editor_pipeline_description.depth_stencil.depth_write_enabled = true;
    editor_pipeline_description.rasterizer.cull_mode = SG_CULLMODE_NONE;
    editor_pipeline_description.label = "editor";

    sg_init_pipeline(graphics_state.editor_pipeline, &editor_pipeline_description);


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


#if IMGUI_HAS_DOCK
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	colors[ImGuiCol_DockingPreview]         = ImVec4(1.000f, 0.391f, 0.000f, 0.781f);
	colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
#endif

    load();

    last_tick_time = ws_time_current();
    last_render_time = ws_time_current();
}

static void frame(void) {
    sfetch_dowork();

    double current_tick_time = ws_time_current();
    double delta_tick_time = current_tick_time - last_tick_time;
    tick(delta_tick_time);
    last_tick_time = current_tick_time;
    
    simgui_new_frame(sapp_width(), sapp_height(), 1.0f/60.0f);
#if IMGUI_HAS_DOCK
    
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGui::SetNextWindowPos(ImVec2(0,0));
        ImGui::SetNextWindowSize(ImVec2(sapp_width(), sapp_height()));
        ImGui::Begin("MainWindow", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoBringToFrontOnFocus);

        ImGuiID dockspace_id = ImGui::GetID("RootDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
    }
#endif

    double current_render_time = ws_time_current();
    double delta_render_time = current_render_time - last_render_time;

    ws_sprite_batcher_reset();

    sg_begin_pass(graphics_state.game_pass, &graphics_state.game_pass_action);
    sg_apply_pipeline(graphics_state.game_pipeline);

    render(delta_render_time);

    sg_push_debug_group("sprite-batcher");
    ws_sprite_batcher_finish();
    sg_pop_debug_group();

    sg_end_pass();
    last_render_time = current_render_time;

    sg_imgui_draw(&graphics_state.sg_imgui);

    auto &sg_imgui = graphics_state.sg_imgui;
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Build")) {
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Tools")) {
            ImGui::MenuItem("Buffers", 0, &sg_imgui.buffers.open);
            ImGui::MenuItem("Images", 0, &sg_imgui.images.open);
            ImGui::MenuItem("Shaders", 0, &sg_imgui.shaders.open);
            ImGui::MenuItem("Pipelines", 0, &sg_imgui.pipelines.open);
            ImGui::MenuItem("Passes", 0, &sg_imgui.passes.open);
            ImGui::MenuItem("Frame Capture", 0, &sg_imgui.capture.open);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::Begin("Scene");
        ImGui::Text("Tick Delta: %3.7f", delta_tick_time);
        ImGui::Text("Render Delta: %3.7f", delta_render_time);
        if(ImGui::CollapsingHeader("Entity Types", ImGuiTreeNodeFlags_DefaultOpen ))
        {
        }
        if(ImGui::CollapsingHeader("Maps", ImGuiTreeNodeFlags_DefaultOpen ))
        {
        }
    ImGui::End();

    ImGui::Begin("Assets");
        if(ImGui::CollapsingHeader("Sprites", ImGuiTreeNodeFlags_DefaultOpen ))
        {
        }
        if(ImGui::CollapsingHeader("Scripts", ImGuiTreeNodeFlags_DefaultOpen ))
        {
        }
        if(ImGui::CollapsingHeader("Sounds", ImGuiTreeNodeFlags_DefaultOpen ))
        {
        }
        if(ImGui::CollapsingHeader("Entity Types", ImGuiTreeNodeFlags_DefaultOpen ))
        {
        }
        if(ImGui::CollapsingHeader("Maps", ImGuiTreeNodeFlags_DefaultOpen ))
        {
        }
    ImGui::End();

#if IMGUI_HAS_DOCK
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGui::End();
    }
#endif

    int game_view_width = 512;
    int game_view_height = 384;
    int game_view_offset_x = sapp_width()/2 - 512/2;
    int game_view_offset_y = sapp_height()/2 - game_view_height/2;
    ImGui::SetNextWindowPos(ImVec2(game_view_offset_x, game_view_offset_y), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(game_view_width, game_view_height));
    ImGui::Begin("Game View", nullptr, ImGuiWindowFlags_NoScrollbar);
        ImGui::Image((ImTextureID)(intptr_t)color_img.id, ImVec2(game_view_width, game_view_height), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.25f));
        // auto draw_list = ImGui::GetWindowDrawList();
        // draw_list->AddCallback([](const ImDrawList* dl, const ImDrawCmd* cmd) {
        //     const int cx = (int) cmd->ClipRect.x;
        //     const int cy = (int) cmd->ClipRect.y;
        //     const int cw = (int) (cmd->ClipRect.z - cmd->ClipRect.x);
        //     const int ch = (int) (cmd->ClipRect.w - cmd->ClipRect.y);
        // }, nullptr);
    ImGui::End();

    sg_begin_default_pass(&graphics_state.default_pass, sapp_width(), sapp_height());
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

    graphics_state.game_pass = {0};
    graphics_state.default_pass = {0};
    graphics_state.game_pass_action = {0};

    auto &game_pass_action = graphics_state.game_pass_action;
    game_pass_action.colors[0].action = SG_ACTION_CLEAR;
    game_pass_action.colors[0].val[0] = 1.0f;
    game_pass_action.colors[0].val[1] = 1.0f;
    game_pass_action.colors[0].val[2] = 1.0f;
    game_pass_action.colors[0].val[3] = 1.0f;

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

double ws_time_current()
{
    return (double)stm_sec(stm_now());
}

void ws_display_set_clear_color( ws_color_t color )
{
    auto &game_pass_action = graphics_state.game_pass_action;
    game_pass_action.colors[0].action = SG_ACTION_CLEAR;
    game_pass_action.colors[0].val[0] = color.r;
    game_pass_action.colors[0].val[1] = color.g;
    game_pass_action.colors[0].val[2] = color.b;
    game_pass_action.colors[0].val[3] = color.a;
}