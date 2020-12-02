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
graphics_state_t graphics_state = {0};

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

bool is_pot(uint64_t n)
{
    return (!(n & (n - 1)));
}

uint64_t npot(uint64_t n)
{
    n |= n >> 1;
    n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	n |= n >> 32;
  return n + 1;
}

static void initialize_game_pass()
{
    graphics_state.render_buffers[ws_render_buffers::frame] = sg_alloc_image();
    graphics_state.render_buffers[ws_render_buffers::depth] = sg_alloc_image();

    sg_image_desc img_desc = {0};
    img_desc.render_target = true;
    img_desc.width = graphics_state.render_target_width;
    img_desc.height = graphics_state.render_target_height;
    img_desc.pixel_format = SG_PIXELFORMAT_RGBA8;
    img_desc.min_filter = SG_FILTER_NEAREST;
    img_desc.mag_filter = SG_FILTER_NEAREST;
    img_desc.wrap_u = SG_WRAP_REPEAT;
    img_desc.wrap_v = SG_WRAP_REPEAT;
    img_desc.sample_count = 0;
    img_desc.label = "color-image";
    
    sg_init_image(graphics_state.render_buffers[ws_render_buffers::frame], &img_desc);

    img_desc.pixel_format = SG_PIXELFORMAT_DEPTH;
    img_desc.label = "depth-image";

    sg_init_image(graphics_state.render_buffers[ws_render_buffers::depth], &img_desc);

    sg_pass_desc pass_description = {0};
    pass_description.color_attachments[0].image = graphics_state.render_buffers[ws_render_buffers::frame];
    pass_description.depth_stencil_attachment.image = graphics_state.render_buffers[ws_render_buffers::depth];
    pass_description.label = "game-pass";

    graphics_state.game_pass = sg_make_pass(&pass_description);
}

static void initialize_pipelines()
{
    graphics_state.pipelines[ws_pipelines::game] = sg_alloc_pipeline();
    graphics_state.pipelines[ws_pipelines::editor] = sg_alloc_pipeline();

    sg_pipeline_desc pipeline_desc = {0};

    pipeline_desc = {0};
    pipeline_desc.shader = graphics_state.shaders[ws_shaders::offscreen];
    pipeline_desc.layout.buffers[0].stride = sizeof(ws_vertex_t);
    pipeline_desc.layout.attrs[ATTR_vs_pos].format = SG_VERTEXFORMAT_FLOAT3;
    pipeline_desc.layout.attrs[ATTR_vs_color0].format = SG_VERTEXFORMAT_FLOAT4;
    pipeline_desc.layout.attrs[ATTR_vs_texcoord0].format = SG_VERTEXFORMAT_FLOAT2;
    pipeline_desc.blend.enabled = true;
    pipeline_desc.blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
    pipeline_desc.blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    pipeline_desc.blend.color_write_mask = SG_COLORMASK_RGB;
    pipeline_desc.blend.color_format = SG_PIXELFORMAT_RGBA8;
    pipeline_desc.blend.depth_format = SG_PIXELFORMAT_DEPTH;
    pipeline_desc.index_type = SG_INDEXTYPE_UINT16;
    pipeline_desc.depth_stencil.depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL;
    pipeline_desc.depth_stencil.depth_write_enabled = true;
    pipeline_desc.rasterizer.cull_mode = SG_CULLMODE_NONE;
    pipeline_desc.label = "game";

    sg_init_pipeline(graphics_state.pipelines[ws_pipelines::game], &pipeline_desc);

    pipeline_desc.shader = graphics_state.shaders[ws_shaders::standard];
    pipeline_desc.label = "editor";

    sg_init_pipeline(graphics_state.pipelines[ws_pipelines::editor], &pipeline_desc);
}

static void setup_imgui_style()
{
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
}
static void init(void) {
    
    // init memory for tagged heap allocator
    if(ws_tagged_heap_init() < 0)
    {
        // todo(Wynter): handle case where we can't init allocator memory
    }

    printf("Initialized tagged heap allocator\n");
    printf("  address: %" PRIxPTR "\n", (uintptr_t) ws_tagged_heap_get_base());
    printf("  block size: %" PRIxPTR "\n", (uintptr_t) ws_tagged_heap_get_block_size());
    printf("  allocate 1 block: %" PRIxPTR "\n", (uintptr_t) ws_tagged_heap_alloc_block(32));
    printf("  allocate 4 blocks: %" PRIxPTR "\n", (uintptr_t) ws_tagged_heap_alloc_n_blocks(32, 4));
    printf("  allocate 1 blocks: %" PRIxPTR "\n", (uintptr_t) ws_tagged_heap_alloc_block(32));

    sg_desc app_description = {0};
    app_description.context = sapp_sgcontext();
    sg_setup(&app_description);

    simgui_desc_t simgui_description = {0};
    simgui_description.dpi_scale = sapp_dpi_scale();
    
    simgui_setup(&simgui_description);
    sg_imgui_init(&graphics_state.sg_imgui);

    graphics_state.shaders[ws_shaders::standard] = sg_make_shader(default_shader_desc());
    graphics_state.shaders[ws_shaders::offscreen] = sg_make_shader(default_shader_desc());

    graphics_state.game_view_width = system_settings.screen.width;
    graphics_state.game_view_height = system_settings.screen.height;
    graphics_state.render_target_width = npot(graphics_state.game_view_width);
    graphics_state.render_target_height = npot(graphics_state.game_view_height);
    graphics_state.render_target_valid = true;
    
    initialize_game_pass();
    initialize_pipelines();

    setup_imgui_style();

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

    // if render target invalidated
    if(!graphics_state.render_target_valid)
    {
        auto &w = graphics_state.game_view_width;
        auto &h = graphics_state.game_view_height;

        graphics_state.render_target_width = is_pot(w) ? w : npot(w);
        graphics_state.render_target_height = is_pot(h) ? h : npot(h);

        if(graphics_state.render_target_width < 32) graphics_state.render_target_width = 32;
        if(graphics_state.render_target_height < 32) graphics_state.render_target_height = 32;

        printf("Resizing view to %dx%d\n", w, h);
        printf("Resizing framebuffer to %dx%d\n", graphics_state.render_target_width, graphics_state.render_target_height);

        sg_image old_render_target = graphics_state.render_buffers[ws_render_buffers::frame];
        sg_image old_depth_buffer = graphics_state.render_buffers[ws_render_buffers::depth];
        sg_pass old_pass = graphics_state.game_pass;

        initialize_game_pass();

        sg_destroy_pass(old_pass);
        sg_destroy_image(old_render_target);
        sg_destroy_image(old_depth_buffer);
        graphics_state.render_target_valid = true;
    }
    
    simgui_new_frame(sapp_width(), sapp_height(), delta_tick_time);
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
    sg_apply_pipeline(graphics_state.pipelines[ws_pipelines::game]);

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
            ImGui::MenuItem("New");
            ImGui::MenuItem("Open");
            ImGui::MenuItem("Save");
            ImGui::Separator();
            ImGui::MenuItem("Export");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Assets")) {
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

    int game_view_offset_x = sapp_width()/2 - graphics_state.game_view_width/2;
    int game_view_offset_y = sapp_height()/2 - graphics_state.game_view_height/2;
    ImGui::SetNextWindowPos(ImVec2(game_view_offset_x, game_view_offset_y), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(graphics_state.game_view_width, graphics_state.game_view_height), ImGuiCond_FirstUseEver);
    ImGui::Begin("Game View", nullptr, ImGuiWindowFlags_NoScrollbar);

        // calculate window center and image size (fixed aspect ratio)
        ImVec2 currentCursorPos = ImGui::GetCursorPos();
        ImVec2 contentRegion = ImGui::GetContentRegionAvail();
        ImVec2 imageRegion = ImVec2(0,0);
        float gameAspectRatio = (float)graphics_state.game_view_width / (float)graphics_state.game_view_height;
        float windowAspectRatio = contentRegion.x / contentRegion.y;
        if(windowAspectRatio > gameAspectRatio)
        {
            imageRegion = ImVec2(graphics_state.game_view_width * (contentRegion.y/graphics_state.game_view_height), contentRegion.y);
        }
        else
        {
            imageRegion = ImVec2(contentRegion.x, graphics_state.game_view_height * (contentRegion.x/graphics_state.game_view_width));
        }
        ImVec2 cursorPos(currentCursorPos.x + (contentRegion.x - imageRegion.x) * 0.5f, currentCursorPos.y + (contentRegion.y - imageRegion.y) * 0.5f);


        ImGui::SetCursorPos(cursorPos);

        ImVec2 view_dims = {
            (float)graphics_state.game_view_width / (float)graphics_state.render_target_width, 
            (float)graphics_state.game_view_height / (float)graphics_state.render_target_height
        };

        ImGui::Image((ImTextureID)(intptr_t)graphics_state.render_buffers[ws_render_buffers::frame].id, imageRegion, ImVec2(0.0f, 1.0f), ImVec2(view_dims.x, 1.0f - view_dims.y));

        graphics_state.game_view_width = contentRegion.x;
        graphics_state.game_view_height = contentRegion.y;

        if( (graphics_state.game_view_width > graphics_state.render_target_width) ||
            (graphics_state.game_view_width < graphics_state.render_target_width / 2) ||
            (graphics_state.game_view_height > graphics_state.render_target_height) ||
            (graphics_state.game_view_height < graphics_state.render_target_height / 2))
        {
            graphics_state.render_target_valid = false;
        }
        
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
    system_settings.screen.hidpi = true;
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
    description.width = 1280;
    description.height = 720;
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