#include <TaggedHeap.h>
#include <cstring>
#include <cstdint>
#include <stdio.h>
#include <inttypes.h>

#ifdef __APPLE__
#define PLATFORM_HAS_MMAP 1
#define PLATFORM_HAS_VIRTUALALLOC 0
#elif defined(_WIN32)
#define PLATFORM_HAS_MMAP 0
#define PLATFORM_HAS_VIRTUALALLOC 1
#elif defined(__linux__)
#define PLATFORM_HAS_MMAP 1
#define PLATFORM_HAS_VIRTUALALLOC 0
#elif defined(__EMSCRIPTEN__)
#define PLATFORM_HAS_MMAP 1
#define PLATFORM_HAS_VIRTUALALLOC 0
#endif

#if PLATFORM_HAS_MMAP
#include <sys/mman.h>
#endif

#if PLATFORM_HAS_VIRTUALALLOC
#include <windows.h>
#endif


constexpr uintptr_t tagged_heap_fixed_addr          = 0x200000000;
constexpr size_t tagged_heap_size                   = 0x00400000;   // 16MB
constexpr size_t tagged_heap_block_size             = 0x1000;       // 4KB
constexpr size_t tagged_heap_block_count            = tagged_heap_size / tagged_heap_block_size;
constexpr ws_tag_t tagged_heap_internal_tag         = 0xFFFFFFFFFFFFFFFF;

struct ws_tagged_heap_t {
    ws_tag_t block_tags[tagged_heap_block_count];
};

static ws_tagged_heap_t* ws_tagged_heap_ptr = nullptr;

constexpr size_t tagged_heap_reserved_block_count   = (sizeof(ws_tagged_heap_t) + (tagged_heap_block_size - 1)) / tagged_heap_block_size;

int ws_tagged_heap_init()
{
#if PLATFORM_HAS_MMAP == 1
    // allocate large memory block
    ws_tagged_heap_ptr = (ws_tagged_heap_t*) mmap((void*)tagged_heap_fixed_addr, tagged_heap_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    if(ws_tagged_heap_ptr == MAP_FAILED)
    {
        // todo(Wynter): handle outputting the error here
        return -1;
    }
#elif PLATFORM_HAS_VIRTUALALLOC == 1
    ws_tagged_heap_ptr = (ws_tagged_heap_t*)VirtualAlloc(0, tagged_heap_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    if (ws_tagged_heap_ptr == nullptr)
    {
        // todo(Wynter): handle outputting the error here
        // note: Use GetLastError on Windows
        return -1;
    }
#else
    #error Platform does not support mmap or VirtualAlloc functionality
#endif

    // clear tagged heap structure at base address
    memset(ws_tagged_heap_ptr, 0, sizeof(ws_tagged_heap_t));
    ws_tagged_heap_ptr->block_tags[0] = tagged_heap_internal_tag;
    return 0;
}

void* ws_tagged_heap_get_base()
{
    return (void*)ws_tagged_heap_ptr;
}

size_t ws_tagged_heap_get_block_size()
{
    return tagged_heap_block_size;
}

void* ws_tagged_heap_alloc_block(ws_tag_t tag)
{
    for(size_t i = tagged_heap_reserved_block_count; i < tagged_heap_block_count; i++)
    {
        if(ws_tagged_heap_ptr->block_tags[i] == 0)
        {
            ws_tagged_heap_ptr->block_tags[i] = tag;
            void* block_address = (void*)((uintptr_t) ws_tagged_heap_ptr + (uintptr_t) i * (uintptr_t)tagged_heap_block_size);
#if WS_DEBUG == 1
            printf("[ws_tagged_heap_alloc_block] a: %" PRIxPTR" p: %" PRIxPTR" i: %" PRIxPTR" s: %" PRIxPTR" \n", 
                (uintptr_t) block_address, (uintptr_t) ws_tagged_heap_ptr, (uintptr_t) i, (uintptr_t) tagged_heap_block_size);
#endif
            return block_address;
        }
    }
    return nullptr;
}

void* ws_tagged_heap_alloc_n_blocks(ws_tag_t tag, size_t blocks)
{
    ws_tag_t *tags = ws_tagged_heap_ptr->block_tags;

    // always start after the reserved blocks
    size_t start = tagged_heap_reserved_block_count;
    size_t blocks_found = 0;
    
    // we can calculate an end point for our search so as not to iterate more than necessary
    size_t end = tagged_heap_block_count - blocks;

    // if we try to allocate more blocks than are available to the allocator, return nullptr
    if(blocks > tagged_heap_block_count - 1)
    {
        return nullptr;
    }

    // loop as long as our starting position doesn't hit our end
    while(start < end)
    {
        if(tags[start + blocks_found] == 0)
        {
            blocks_found++;
            
            if(blocks_found == blocks)
            {
                // since we've found the number of blocks we need, tag them all
                for(size_t i = start; i < start + blocks_found; i++)
                {
                    tags[i] = tag;
                }

                void* block_address = (void*)((uintptr_t) ws_tagged_heap_ptr + (uintptr_t) start * (uintptr_t)tagged_heap_block_size);
                return block_address;
            }
        }
        else
        {
            blocks_found = 0;
            start++;
        }
    }
    return nullptr;
}

void ws_tagged_heap_free_block(void* block_address)
{
    // todo(Wynter): clean up these commented out code segments once this is verified to work correctly

    size_t block_number = ((uintptr_t)block_address - (uintptr_t)ws_tagged_heap_ptr) / tagged_heap_block_size + tagged_heap_reserved_block_count;

    // if(block_address >= start && block_address <= end )
    
    // ignore reserved blocks
    if(block_number > tagged_heap_reserved_block_count && block_number < tagged_heap_block_count )
    {
        ws_tagged_heap_ptr->block_tags[block_number] = 0;
    }
}

void ws_tagged_heap_free_tag(ws_tag_t tag)
{
    // always start after reserved blocks
    for(size_t i = tagged_heap_reserved_block_count; i < tagged_heap_block_count; i++)
    {
        if(ws_tagged_heap_ptr->block_tags[i] == tag)
        {
            ws_tagged_heap_ptr->block_tags[i] = 0;
        }
    }
}