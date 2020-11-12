#ifndef PR_COMMON
#define PR_COMMON

#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <ctype.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;

typedef float f32;
typedef double f64;


#define align(x) __attribute__ ((aligned(x))) //use stdalign.h?

#define array_count(x) (sizeof(x) / sizeof(x[0]))

#define EPSILON 1e-5f
#define PI 3.1415927f

#define TO_RADIANS(degrees) ((PI / 180) * (degrees))
#define TO_DEGREES(radians) ((180 / PI) * (radians))

typedef struct Buffer {
    u8 *data;
    u32 size;
    u32 max_size;
} Buffer;

typedef Buffer Array;

typedef struct Allocator {
    u8 *perm_memory_base;
    size_t perm_memory_used;
    size_t perm_memory_size;
    
    bool begin_frame_allocation;
    u8 *frame_memory_base;
    size_t frame_memory_used;
    size_t frame_memory_size;
    
    u8 *memory;
} Allocator;

Allocator *g_allocator;

size_t convert_gibibytes_to_bytes(f32 size) {
    return (size_t)(size * (f32)pow(1024, 3));
}

u8 *allocate_perm_aligned(Allocator *allocator, size_t allocation_size, u32 alignment) {
    uintptr_t alignment_mask = alignment - 1;
    u8 *memory = allocator->perm_memory_base + allocator->perm_memory_used;
    uintptr_t offset = 0;
    
    if ((uintptr_t)memory & alignment_mask) {
        offset = (uintptr_t)alignment - ((uintptr_t)memory & alignment_mask);
    }
    
    allocation_size += offset;
    
    assert(allocation_size + allocator->perm_memory_used <= allocator->perm_memory_size);
    //memory += offset;
    allocator->perm_memory_used += allocation_size;
    return memory + offset;
}

void begin_frame_allocation(Allocator *allocator) {
    allocator->begin_frame_allocation = true;
}

u8 *allocate_frame_aligned(Allocator *allocator, size_t allocation_size, u32 alignment) {
    assert(allocator->begin_frame_allocation);
    uintptr_t alignment_mask = alignment - 1;
    u8 *memory = allocator->frame_memory_base + allocator->frame_memory_used;
    uintptr_t offset = 0;
    
    if ((uintptr_t)memory & alignment_mask) {
        offset = (uintptr_t)alignment - ((uintptr_t)memory & alignment_mask);
    }
    
    allocation_size += offset;
    
    assert(allocation_size + allocator->frame_memory_used <= allocator->frame_memory_size);
    //memory += offset;
    allocator->frame_memory_used += allocation_size;
    return memory + offset;
}

u8 *allocate_perm(Allocator *allocator, size_t allocation_size) {
    u32 alignment = 4;
    uintptr_t alignment_mask = alignment - 1;
    u8 *memory = allocator->perm_memory_base + allocator->perm_memory_used;
    uintptr_t offset = ((uintptr_t)memory & alignment_mask);
    if (offset != 0) {
        memory -= offset;
        memory += 1;
    }
    
    assert(memory + allocation_size <= allocator->perm_memory_base +  allocator->perm_memory_size);
    allocator->perm_memory_used += allocation_size;
    return memory;
}

u8 *allocate_frame(Allocator *allocator, size_t allocation_size) {
    assert(allocator->begin_frame_allocation);
    u32 alignment = 4;
    uintptr_t alignment_mask = alignment - 1;
    u8 *memory = allocator->frame_memory_base + allocator->frame_memory_used;
    uintptr_t offset = 0;
    
    if ((uintptr_t)memory & alignment_mask) {
        offset = (uintptr_t)memory - ((uintptr_t)memory & alignment_mask);
    }
    
    allocation_size += offset;
    
    assert(allocation_size + allocator->frame_memory_used <= allocator->frame_memory_size);
    memory += offset;
    allocator->frame_memory_used += allocation_size;
    return memory;
}

void reset_frame_memory(Allocator *allocator) {
    allocator->frame_memory_used = 0;
}

Allocator *create_allocator(float perm_memory_size_in_gigabytes, float frame_memory_size_in_gigabytes) {
    size_t perm_memory_size = convert_gibibytes_to_bytes(perm_memory_size_in_gigabytes);
    size_t frame_memory_size =  convert_gibibytes_to_bytes(frame_memory_size_in_gigabytes);
    u8 *memory = malloc(sizeof(Allocator) + perm_memory_size + frame_memory_size);
    assert(memory);
    Allocator *allocator = (Allocator *)memory;
    allocator->perm_memory_size = perm_memory_size;
    allocator->frame_memory_size = frame_memory_size;
    allocator->perm_memory_base = memory + sizeof(Allocator);
    allocator->frame_memory_base = memory + perm_memory_size + sizeof(Allocator);
    allocator->frame_memory_used = 0;
    allocator->perm_memory_used = 0;
    allocator->memory = memory;
    
    g_allocator = allocator;
    
    return allocator;
}

void free_allocator(Allocator *allocator) {
    free(allocator->memory);
}

#endif