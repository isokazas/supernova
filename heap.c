#include <stdint.h>
#include <stddef.h>

/* Simple bump allocator on top of page allocator */
#define HEAP_START 0x10000  /* Arbitrary safe location */
#define HEAP_SIZE  (64 * 4096)  /* 256KB heap */

static uint8_t heap[HEAP_SIZE];
static size_t heap_offset = 0;

void* malloc(size_t size) {
    if (heap_offset + size > HEAP_SIZE) {
        return NULL;  /* Out of memory */
    }
    void *ptr = (void*)&heap[heap_offset];
    heap_offset += size;
    return ptr;
}

void free(void *ptr) {
    /* Simple bump allocator doesn't free; implement linked-list allocator for real OS */
    (void)ptr;
}

void* calloc(size_t num, size_t size) {
    size_t total = num * size;
    void *ptr = malloc(total);
    if (ptr) {
        for (size_t i = 0; i < total; i++) {
            ((uint8_t*)ptr)[i] = 0;
        }
    }
    return ptr;
}
