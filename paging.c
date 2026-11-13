#include <stdint.h>
#include <stddef.h>

/* Simple bitmap page allocator */
#define PAGE_SIZE 4096
#define NUM_PAGES 256  /* 1MB / 4KB = covers low 1MB for now */

static uint8_t page_bitmap[NUM_PAGES / 8];  /* 1 bit per page */

static void bitmap_set(int page) {
    page_bitmap[page / 8] |= (1 << (page % 8));
}

static void bitmap_clear(int page) {
    page_bitmap[page / 8] &= ~(1 << (page % 8));
}

static int bitmap_test(int page) {
    return (page_bitmap[page / 8] >> (page % 8)) & 1;
}

/* Allocate a page, return its physical address */
void* page_alloc(void) {
    for (int i = 0; i < NUM_PAGES; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            return (void*)(i * PAGE_SIZE);
        }
    }
    return NULL;  /* Out of memory */
}

/* Free a page */
void page_free(void *addr) {
    int page = (uintptr_t)addr / PAGE_SIZE;
    if (page >= 0 && page < NUM_PAGES) {
        bitmap_clear(page);
    }
}

/* Get page allocation status (for debug) */
int page_count_free(void) {
    int count = 0;
    for (int i = 0; i < NUM_PAGES; i++) {
        if (!bitmap_test(i)) count++;
    }
    return count;
}
