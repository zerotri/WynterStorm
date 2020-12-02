#include <WynterStorm.h>
#include <map>
#include <iostream>
#include <vector>
#include <TaggedHeap.h>

// ws_handle_t null_handle()
// {
//     ws_handle_t handle;
//     handle.address = reinterpret_cast<uintptr_t>(nullptr); // - ws_memory_base
//     return handle;
// }

// template<typename T>
// ws_handle_t to_handle(T* pointer)
// {
//     ws_handle_t handle;
//     handle.address = reinterpret_cast<uintptr_t>(pointer); // - ws_memory_base
//     return handle;
// }

// template<typename T>
// T* to_pointer(ws_handle_t handle)
// {
//     return reinterpret_cast<T*>(handle.address); // + ws_memory_base;
// }

  constexpr uint32_t val_32_const = 0x811c9dc5;
  constexpr uint32_t prime_32_const = 0x1000193;
  constexpr uint64_t val_64_const = 0xcbf29ce484222325;
  constexpr uint64_t prime_64_const = 0x100000001b3;

  inline constexpr uint32_t hash_32_fnv1a_const(const char* const str, const uint32_t value = val_32_const) noexcept {
      return (str[0] == '\0') ? value : hash_32_fnv1a_const(&str[1], (value ^ uint32_t(str[0])) * prime_32_const);
  }

  inline constexpr uint64_t hash_64_fnv1a_const(const char* const str, const uint64_t value = val_64_const) noexcept {
      return (str[0] == '\0') ? value : hash_64_fnv1a_const(&str[1], (value ^ uint64_t(str[0])) * prime_64_const);
  }

// sokol
#include "platform_sokol_internal.h"
#include "HandmadeMath.h"
#include "default.shader.h"

// needed for fopen in cute_asprite
#include <cstdio>
#define CUTE_ASEPRITE_IMPLEMENTATION
#include <cute_aseprite.h>

struct ws_internal_sprite_rect_t {
    float x, y, w, h;
};

struct ws_internal_sprite_t {
    uint16_t map_index;
    ase_t* ase;
    sg_image id;
    // sg_image_desc* descriptions;
	int width, height;
    int atlas_width, atlas_height;
	int type;
	int flags;
    // temporary until atlasing is implemented
    bool allocatedForSprite;
    std::vector<ws_vertex_t> vertices;
    std::vector<uint16_t> indices;
    std::vector<ws_internal_sprite_rect_t> frames;
    sg_buffer vertex_buffer;
    sg_buffer index_buffer;
    int vertexCount;
    int indexCount;
    bool isLoaded;
    const char* label;
};

std::map<int16_t, struct ws_internal_sprite_t> sprites;

// calculating size needed to store full sprite atlas
// int npot(int n) {
//     int i = 0;
//     for (--n; n > 0; n >>= 1) {
//         i++;
//     }
//     return 1 << i;
// }
//

//
// sqrt take number of frames, then ceil result
// take outcome and multiply by image w and h to get w2 and h2
// use npot(w2) and npot(h2) to get resulting spritesheet size
// 
// image: 12 x 62 = 744px
// images: 19 = 14136px
// ceil(sqrt(19)) == 5

// 12 * 5 = npot(60) = 64
// 63 * 5 = npot(315) = 512
// 18900/32768px

void __hacky_copy_buffer_rect(char* src, int src_x, int src_y, int src_stride,
                        char* dest, int dest_x, int dest_y, int dest_stride,
                        int w, int h, int bpp)
{
    char* src_ptr = (char*)(uintptr_t)src + (src_y * src_stride) + (src_x * bpp);
    char* dest_ptr = (char*)(uintptr_t)dest + (dest_y * dest_stride) + (dest_x * bpp);

    for(int y = 0; y < h; y++)
    {
        memcpy(dest_ptr, src_ptr, w * bpp);
        src_ptr += src_stride;
        dest_ptr += dest_stride;
    }
}


ws_handle_t ws_sprite_load( const char *file_path )
{
    static uint16_t last_map_index = 0;
    static char buffer[4 * 1024];
    sfetch_request_t sprite_fetch_request = {0};

    ws_internal_sprite_t sprite = {0};

    sprite.map_index = last_map_index;
    sprite.isLoaded = false;
    sprite.id = sg_alloc_image();
    sprite.label = file_path;

    sprites[last_map_index] = sprite;

    sprite_fetch_request.path = file_path;
    sprite_fetch_request.buffer_ptr = buffer;
    sprite_fetch_request.buffer_size = sizeof(buffer);
    sprite_fetch_request.user_data_ptr = &last_map_index;
    sprite_fetch_request.user_data_size = sizeof(last_map_index);
    sprite_fetch_request.callback = [](const sfetch_response_t* response) {
        std::cout << "fetch callback" << std::endl;
        if (response->fetched) {
            uint16_t index = *(uint16_t*) response->user_data;
            ws_internal_sprite_t sprite = sprites[index];
            sprite.ase = cute_aseprite_load_from_memory(response->buffer_ptr, response->buffer_size, nullptr);
            int w = sprite.ase->w;
            int h = sprite.ase->h;
            int bpp = 4;
            sprite.width = w;
            sprite.height = h;

            // calculate atlas size
            int num_frames = sprite.ase->frame_count;
            int atlas_multiplier = (int)ceil(sqrt((float)num_frames));
            int atlas_width = w * atlas_multiplier;
            int atlas_height = h * atlas_multiplier;
            int atlas_buffer_size = atlas_width * atlas_height * bpp;
            int atlas_heap_blocks = (int)ceil((float)atlas_buffer_size / (float)ws_tagged_heap_get_block_size());

            char* atlas_buffer = (char*)ws_tagged_heap_alloc_n_blocks(hash_32_fnv1a_const("ws_sprite_load"), atlas_heap_blocks);
            if(atlas_buffer == nullptr)
            {
                // todo(Wynter): Handle failure to alloc blocks
            }

            // messy spritesheet generation code
            // todo(Wynter): clean this up
            int i = 0;
            int x = 0;
            int y = 0;
            while( i < num_frames)
            {
                char *pixel_data = (char*)sprite.ase->frames[i].pixels;
                __hacky_copy_buffer_rect(pixel_data, 0, 0, w * bpp, atlas_buffer, x * w, y * h, atlas_width * bpp, w, h, bpp);
                

                ws_internal_sprite_rect_t frame_rect = {((float)x) * w, ((float)y) * h, (float)w, (float)h};
                sprite.frames.push_back(frame_rect);

                i++;
                x++;

                if(i % atlas_multiplier == 0)
                {
                    y++;
                    x = 0;
                }
            }

            sprite.atlas_width = atlas_width;
            sprite.atlas_height = atlas_height;

            sg_subimage_content subimage;
            subimage.ptr = atlas_buffer;
            subimage.size = atlas_width * atlas_height * bpp;
            
            // set description to zero or sokol will complain about it not being initialized
            // NOTE(Wynter): will this work in MSVC???
            // auto &description = sprite.description;
            auto description = sg_image_desc {0};
            description.width = atlas_width;
            description.height = atlas_height;
            description.pixel_format = SG_PIXELFORMAT_RGBA8;
            description.min_filter = SG_FILTER_NEAREST;
            description.mag_filter = SG_FILTER_NEAREST;
            description.content.subimage[0][0] = subimage;
            description.label = sprite.label;

            sprite.vertices.clear();
            sprite.indices.clear();

            sg_buffer_desc vertex_buffer_desc = {0};
            vertex_buffer_desc.type = SG_BUFFERTYPE_VERTEXBUFFER;
            vertex_buffer_desc.usage = SG_USAGE_STREAM;
            vertex_buffer_desc.size = sizeof(ws_vertex_t) * (1<<16);
            vertex_buffer_desc.label = response->path;

            sg_buffer_desc index_buffer_desc = {0};
            index_buffer_desc.type = SG_BUFFERTYPE_INDEXBUFFER;
            index_buffer_desc.usage = SG_USAGE_STREAM;
            index_buffer_desc.size = sizeof(uint16_t) * ((1<<16) * 3);
            index_buffer_desc.label = response->path;

            sprite.vertex_buffer = sg_make_buffer(&vertex_buffer_desc);
            sprite.index_buffer = sg_make_buffer(&index_buffer_desc);
            sg_init_image(sprite.id, &description);

            sprite.map_index = index;
            sprite.isLoaded = true;

            sprites[index] = sprite;

            std::cout << "Loaded sprite from " << response->path << std::endl;
            std::cout << "\t Index\t" << sprite.map_index << std::endl;
            std::cout << "\t Id\t" << sprite.id.id << std::endl;
            std::cout << "\t Layers\t" << sprite.ase->layer_count << std::endl;
            std::cout << "\t Frames\t" << sprite.frames.size() << std::endl;
            std::cout << "\t Width\t" << w << std::endl;
            std::cout << "\t Height\t" << h << std::endl;
            std::cout << "\t Atlas Width\t" << atlas_width << std::endl;
            std::cout << "\t Atlas Height\t" << atlas_height << std::endl;
            std::cout << "\t Frames" << std::endl;
            for( auto &frame : sprite.frames)
            {
                std::cout   << "\t   {"
                            << frame.x << ", "
                            << frame.y << ", "
                            << frame.w << ", "
                            << frame.h << "}" << std::endl;
            }
        }
        
        if (response->finished) {
            // the 'finished'-flag is the catch-all flag for when the request
            // is finished, no matter if loading was successful or failed,
            // so any cleanup-work should happen here...
            if (response->failed) {
                std::cout << "Failed to load file " << response->path << std::endl;
            }
        }
    };

    std::cout << "Fetching " << file_path << std::endl;
    sfetch_send(&sprite_fetch_request);
    ws_handle_t new_handle = { last_map_index++ };
    return new_handle; // to_handle(&sprites[last_map_index++]);
}

void ws_draw_sprite( ws_handle_t sprite, int frame, float x, float y )
{
    auto sprite_iter = sprites.find(sprite.id);
    if(sprite_iter != sprites.end())
    {
        auto &sprite = sprite_iter->second;
        if(!sprite.isLoaded) return;
        if(frame > sprite.frames.size()) return;

        auto &frame_info = sprite.frames[frame];

        auto xe = x + frame_info.w;
        auto ye = y + frame_info.h;

        auto pixel_size_x = 1.0f / sprite.atlas_width;
        auto pixel_size_y = 1.0f / sprite.atlas_height;
        auto u1 = pixel_size_x * frame_info.x;
        auto u2 = pixel_size_x * (frame_info.x + frame_info.w);
        auto v1 = pixel_size_y * frame_info.y;
        auto v2 = pixel_size_y * (frame_info.y + frame_info.h);

        auto &vertices = sprite.vertices;
        auto start_index = vertices.size();
        float z = -5.0f;
        vertices.push_back(ws_vertex_t{  x,  y, z, 1.0f, 1.0f, 1.0f, 1.0f, u1,  v1});
        vertices.push_back(ws_vertex_t{ xe,  y, z, 1.0f, 1.0f, 1.0f, 1.0f, u2,  v1});
        vertices.push_back(ws_vertex_t{ xe, ye, z, 1.0f, 1.0f, 1.0f, 1.0f, u2,  v2});
        vertices.push_back(ws_vertex_t{  x, ye, z, 1.0f, 1.0f, 1.0f, 1.0f, u1,  v2});

        auto &indices = sprite.indices;
        indices.push_back(start_index);
        indices.push_back(start_index+1);
        indices.push_back(start_index+2);
        indices.push_back(start_index+0);
        indices.push_back(start_index+2);
        indices.push_back(start_index+3);
    }
}

void ws_sprite_batcher_reset()
{
    for(auto &sprite : sprites)
    {
        sprite.second.vertices.clear();
        sprite.second.indices.clear();
    }
}

void ws_sprite_batcher_finish()
{
    #define RENDER_SPRITE_BATCHER 1
    #if RENDER_SPRITE_BATCHER
    ImGui::Begin("Sprite Batcher");
        ImGui::Text("Sprites: %lu", sprites.size());
    #endif

    vs_params_t vs_params;
    
    hmm_mat4 proj = HMM_Orthographic(0.0f, (float)sapp_width(), (float)sapp_height(), 0.0f, 0.0f, 10.0f);

    vs_params.mvp = proj;

    for(auto sprite_iter : sprites)
    {
        auto &sprite = sprite_iter.second;
        if(sprite.vertices.size() == 0) continue;
        #if RENDER_SPRITE_BATCHER
        if (ImGui::CollapsingHeader(sprite.label))
        {
            ImGui::Text("Index: %u", sprite_iter.first);

            ImGui::Text("Label: %s", sprite.label);

            ImGui::Text("  vertices: %lu", sprite.vertices.size());
            for(int i = 0; i < sprite.vertices.size(); i++) {
                auto &vert = sprite.vertices[i];
                ImGui::Text("    v: {%f, %f, %f, %f, %f }", vert.x, vert.y, vert.z, vert.u, vert.v);
            }
            ImGui::Text("  indices: %lu", sprite.indices.size());
        }
        #endif

        int vtx_offset = sg_append_buffer(sprite.vertex_buffer, &sprite.vertices[0], sizeof(ws_vertex_t) * sprite.vertices.size());
        int idx_offset = sg_append_buffer(sprite.index_buffer, &sprite.indices[0], sizeof(uint16_t) * sprite.indices.size());

        graphics_state.bind.vertex_buffers[0] = sprite.vertex_buffer;
        graphics_state.bind.index_buffer = sprite.index_buffer;
        graphics_state.bind.vertex_buffer_offsets[0] = vtx_offset;
        graphics_state.bind.index_buffer_offset = idx_offset;
        graphics_state.bind.fs_images[SLOT_tex] = sprite.id;

        sg_apply_viewport(0, 0, sapp_width(), sapp_height(), true);
        sg_apply_pipeline(graphics_state.game_pipeline);
        sg_apply_bindings(&graphics_state.bind);
        sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &vs_params, sizeof(vs_params));

        sg_draw(0, sprite.indices.size(), 1);
    }
    #if RENDER_SPRITE_BATCHER
    ImGui::End();
    #endif
}