#pragma once
#include <cstdint>
#include <stddef.h>

typedef uint64_t ws_tag_t;

int ws_tagged_heap_init();
void* ws_tagged_heap_get_base();
size_t ws_tagged_heap_get_block_size();
void* ws_tagged_heap_alloc_block(ws_tag_t tag);
void* ws_tagged_heap_alloc_n_blocks(ws_tag_t tag, size_t blocks);
void ws_tagged_heap_free_block(void* block_address);
void ws_tagged_heap_free_tag(ws_tag_t tag);