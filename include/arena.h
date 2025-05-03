#ifndef ARENA_H_
#define ARENA_H_

#ifndef ARENA_REGION_DEFAULT_CAPACITY
#define ARENA_REGION_DEFAULT_CAPACITY (8*1024)
#endif

#ifndef ARENA_ASSERT
#include <assert.h>
#define ARENA_ASSERT assert
#endif

#include <stdint.h>
#include <stddef.h>

typedef struct ArenaRegion ArenaRegion;
struct ArenaRegion {
    ArenaRegion *next;
    uint32_t count;
    uint32_t capacity;
    uintptr_t data[];
};

typedef struct Arena {
    ArenaRegion *head;
    ArenaRegion *tail;
} Arena;

void *arena_alloc(Arena *a, size_t size);
void arena_free(Arena *a);
void arena_reset(Arena *a);
void *arena_bufdup(Arena *a, const void *buf, size_t bufsz);

void *arena_backend_heap_alloc(size_t size);
void arena_backend_heap_free(void *buf, size_t bufsz);

#endif // ARENA_H_

#ifdef ARENA_IMPLEMENTATION

ArenaRegion *_create_arena_region(size_t capacity)
{
    size_t size_bytes = sizeof(ArenaRegion) + sizeof(uintptr_t)*capacity;
    ArenaRegion *r = arena_backend_heap_alloc(size_bytes);
    ARENA_ASSERT(r && "`arena_backend_heap_alloc` returns NULL");
    r->next = NULL;
    r->count = 0;
    r->capacity = capacity;
    return r;
}

void _destroy_arena_region(ArenaRegion *r)
{
    arena_backend_heap_free(r, sizeof(ArenaRegion) + sizeof(uintptr_t)*r->capacity);
}

void *arena_alloc(Arena *a, size_t size_in_bytes)
{
    ARENA_ASSERT(a && "Provide a valid argument `a` which is a pointer to `Arena`");
    size_t size = (size_in_bytes + sizeof(uintptr_t) - 1)/sizeof(uintptr_t);
    if (a->head == NULL) {
        ARENA_ASSERT(a->head == NULL);
        size_t capacity = ARENA_REGION_DEFAULT_CAPACITY;
        if (capacity < size) capacity = size;
        a->tail = _create_arena_region(capacity);
        a->head = a->tail;
    }

    while (a->tail->count + size > a->tail->capacity && a->tail->next != NULL) {
        a->tail = a->tail->next;
    }

    if (a->tail->count + size > a->tail->capacity) {
        ARENA_ASSERT(a->tail->next == NULL);
        size_t capacity = ARENA_REGION_DEFAULT_CAPACITY;
        if (capacity < size) capacity = size;
        a->tail->next = _create_arena_region(capacity);
        a->tail = a->tail->next;
    }

    void *result = &a->tail->data[a->tail->count];
    a->tail->count += size;
    return result;
}

void arena_free(Arena *a)
{
    ARENA_ASSERT(a && "Provide a valid argument `a` which is a pointer to `Arena`");
    ArenaRegion *r = a->head;
    while (r) {
        ArenaRegion *r0 = r;
        r = r->next;
        _destroy_arena_region(r0);
    }
    a->head = NULL;
    a->tail = NULL;
}

void arena_reset(Arena *a)
{
    ARENA_ASSERT(a && "Provide a valid argument `a` which is a pointer to `Arena`");
    for (ArenaRegion *r = a->head; r != NULL; r = r->next) {
        r->count = 0;
    }

    a->tail = a->head;
}

void *arena_bufdup(Arena *a, const void *buf, size_t bufsz)
{
    ARENA_ASSERT(a && "Provide a valid argument `a` which is a pointer to `Arena`");
    void *newbuf = arena_alloc(a, bufsz+1);
    ((uint8_t*)newbuf)[bufsz] = 0;
    for(size_t i = 0; i < bufsz; ++i) {
        ((uint8_t*)newbuf)[i] = ((const uint8_t*)buf)[i];
    }
    return newbuf;
}

#ifndef ARENA_NO_PLATFORM
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
void *arena_backend_heap_alloc(size_t size_in_bytes)
{
    void *buf = VirtualAllocEx(
            GetCurrentProcess(),
            NULL,
            size_in_bytes,
            MEM_COMMIT | MEM_RESERVE,
            PAGE_READWRITE);
    if(buf == 0 || buf == INVALID_HANDLE_VALUE) {
        return NULL;
    }
    return buf;
}

void arena_backend_heap_free(void *buf, size_t bufsz)
{
    if(buf == 0 || buf == INVALID_HANDLE_VALUE) {
        return;
    }
    VirtualFreeEx(GetCurrentProcess(),(LPVOID)buf, bufsz, MEM_RELEASE);
}
#endif
#endif // ARENA_NO_PLATFORM

#endif // ARENA_IMPLEMENTATION
