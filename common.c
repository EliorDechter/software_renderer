#ifndef PR_COMMON
#define PR_COMMON

#include <stdint.h>
#include <assert.h>
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




typedef struct Allocator {
    u8 *perm_memory_base;
    size_t perm_memory_used;
    size_t perm_memory_size;
    
    u8 *frame_memory_base;
    size_t frame_memory_used;
    size_t frame_memory_size;
    
} Allocator;


Allocator *g_allocator;


Allocator create_allocator(u8 *memory_buffer, size_t perm_memory_size, size_t frame_memory_size) {
    Allocator allocator = {0};
    allocator.perm_memory_size = perm_memory_size;
    allocator.frame_memory_size = frame_memory_size;
    allocator.perm_memory_base = memory_buffer;
    allocator.frame_memory_base = memory_buffer + perm_memory_size;
    
    return allocator;
}

u8 *allocate_perm_aligned(Allocator *allocator, size_t allocation_size, u32 alignment) {
    uintptr_t alignment_mask = alignment - 1;
    u8 *memory = allocator->perm_memory_base + allocator->perm_memory_used;
    uintptr_t offset = 0;
    
    if ((uintptr_t)memory & alignment_mask) {
        offset = (uintptr_t)memory - ((uintptr_t)memory & alignment_mask);
    }
    
    allocation_size += offset;
    
    assert(allocation_size + allocator->perm_memory_used <= allocator->perm_memory_size);
    memory += offset;
    allocator->perm_memory_used += allocation_size;
    return memory;
}

u8 *allocate_frame_aligned(Allocator *allocator, size_t allocation_size, u32 alignment) {
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
#endif