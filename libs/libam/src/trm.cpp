#include <am.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

const uint32_t heap_size = 0x1000000;
Area heap = { .start = malloc(heap_size), .end = heap.start + heap_size };

void putch(char ch) {
    putchar(ch);
}

void halt(int code) {
    exit(code);
}
