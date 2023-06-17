#ifndef BITMAP_H
#define BITMAP_H
#include "common/types.h"

typedef struct _bitmap_t
{
    int bit_count;
    u8 *bits;
}bitmap_t;

int bitmap_bytes_count(int bit_count);
void bitmap_init(bitmap_t *bitmap, u8 *bits, int count, int init_bit);
int bitmap_get_bit(bitmap_t *bitmap, int index);
void bitamp_set_bit(bitmap_t *bitmap, int index, int count, int bit);
int bitmap_is_set(bitmap_t *bitmap, int index);
int bitmap_alloc_nbits(bitmap_t *bitmap, int bit, int count);

#endif