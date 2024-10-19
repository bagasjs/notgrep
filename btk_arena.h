/*

   `btk_arena.h` - A single headeronly simple arena allocator for C

   GUIDE:
   1. Create the implementation
   ```c
    #include <stdio.h>
    ....
    #define BTK_ARENA_IMPLEMENTATION
    #include "btk_arena.h"
   ```

   2. btk_arena.h contains following macros
    - BTKA_ASSERT - you could redefine this macro to nothing so no assertion will be done
    - BTKA_NO_PLATFORM - this will require you to implement functions that with prefix btka_platform_xxx

*/
#ifndef BTK_ARENA_H_
#define BTK_ARENA_H_

#define BTKA_NULL (void *)0
#define BTKA_STATIC_ASSERT(cond) _Static_assert(cond, #cond)

#ifndef BTKA_REGION_DEFAULT_CAPACITY
#define BTKA_REGION_DEFAULT_CAPACITY (8*1024)
#endif

#ifndef BTKA_ASSERT
#include <assert.h>
#define BTKA_ASSERT assert
#endif

#include <stddef.h>
typedef size_t btka_size_t;

typedef void *btka_uintptr_t;

typedef unsigned char btka_byte_t;
BTKA_STATIC_ASSERT(sizeof(btka_byte_t) == 1 && "Please change the type alias for `btka_byte_t` to any type with size 1 bytes");

typedef struct btk_arena_region btk_arena_region_t;
struct btk_arena_region {
    btk_arena_region_t *next;
    btka_size_t count;
    btka_size_t capacity;
    btka_uintptr_t data[];
};

typedef struct btk_arena {
    btk_arena_region_t *head;
    btk_arena_region_t *tail;
} btk_arena_t;

void *btk_arena_alloc(btk_arena_t *a, btka_size_t size);
void btk_arena_free(btk_arena_t *a);
void btk_arena_reset(btk_arena_t *a);
void *btk_arena_bufdup(btk_arena_t *a, const void *buf, btka_size_t bufsz);

void *btka_platform_map_memory(btka_size_t size);
void btka_platform_unmap_memory(void *buf, btka_size_t bufsz);

#endif // BTK_ARENA_H_

#define BTK_ARENA_IMPLEMENTATION
#ifdef BTK_ARENA_IMPLEMENTATION

btk_arena_region_t *_btka_create_arena_region(btka_size_t capacity)
{
    size_t size_bytes = sizeof(btk_arena_region_t) + sizeof(uintptr_t)*capacity;
    btk_arena_region_t *r = btka_platform_map_memory(size_bytes);
    BTKA_ASSERT(r && "`btka_platform_map_memory` returns NULL");
    r->next = BTKA_NULL;
    r->count = 0;
    r->capacity = capacity;
    return r;
}

void _btka_destroy_arena_region(btk_arena_region_t *r)
{
    btka_platform_unmap_memory(r, sizeof(btk_arena_region_t) + sizeof(uintptr_t)*r->capacity);
}

void *btk_arena_alloc(btk_arena_t *a, size_t size_in_bytes)
{
    BTKA_ASSERT(a && "Provide a valid argument `a` which is a pointer to `btk_arena_t`");
    btka_size_t size = (size_in_bytes + sizeof(btka_uintptr_t) - 1)/sizeof(btka_uintptr_t);
    if (a->head == BTKA_NULL) {
        BTKA_ASSERT(a->head == BTKA_NULL);
        size_t capacity = BTKA_REGION_DEFAULT_CAPACITY;
        if (capacity < size) capacity = size;
        a->tail = _btka_create_arena_region(capacity);
        a->head = a->tail;
    }

    while (a->tail->count + size > a->tail->capacity && a->tail->next != NULL) {
        a->tail = a->tail->next;
    }

    if (a->tail->count + size > a->tail->capacity) {
        BTKA_ASSERT(a->tail->next == NULL);
        size_t capacity = BTKA_REGION_DEFAULT_CAPACITY;
        if (capacity < size) capacity = size;
        a->tail->next = _btka_create_arena_region(capacity);
        a->tail = a->tail->next;
    }

    void *result = &a->tail->data[a->tail->count];
    a->tail->count += size;
    return result;
}

void btk_arena_free(btk_arena_t *a)
{
    BTKA_ASSERT(a && "Provide a valid argument `a` which is a pointer to `btk_arena_t`");
    btk_arena_region_t *r = a->head;
    while (r) {
        btk_arena_region_t *r0 = r;
        r = r->next;
        _btka_destroy_arena_region(r0);
    }
    a->head = NULL;
    a->tail = NULL;
}

void btk_arena_reset(btk_arena_t *a)
{
    BTKA_ASSERT(a && "Provide a valid argument `a` which is a pointer to `btk_arena_t`");
    for (btk_arena_region_t *r = a->head; r != NULL; r = r->next) {
        r->count = 0;
    }

    a->tail = a->head;
}

void *btk_arena_bufdup(btk_arena_t *a, const void *buf, size_t bufsz)
{
    BTKA_ASSERT(a && "Provide a valid argument `a` which is a pointer to `btk_arena_t`");
    void *newbuf = btk_arena_alloc(a, bufsz+1);
    ((btka_byte_t*)newbuf)[bufsz] = 0;
    for(btka_size_t i = 0; i < bufsz; ++i) {
        ((btka_byte_t*)newbuf)[i] = ((const btka_byte_t*)buf)[i];
    }
    return newbuf;
}

#ifndef BTKA_NO_PLATFORM
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
void *btka_platform_map_memory(btka_size_t size_in_bytes)
{
    void *buf = VirtualAllocEx(
            GetCurrentProcess(),
            NULL,
            size_in_bytes,
            MEM_COMMIT | MEM_RESERVE,
            PAGE_READWRITE);
    if(buf == 0 || buf == INVALID_HANDLE_VALUE) {
        return BTKA_NULL;
    }
    return buf;
}

void btka_platform_unmap_memory(void *buf, btka_size_t bufsz)
{
    if(buf == 0 || buf == INVALID_HANDLE_VALUE) {
        return;
    }
    VirtualFreeEx(GetCurrentProcess(),(LPVOID)buf, bufsz, MEM_RELEASE);
}
#endif
#endif // BTKA_NO_PLATFORM

#endif // BTK_ARENA_IMPLEMENTATION
