#include "tools/bitmap.h"
#include "tools/klib.h"

int bitmap_bytes_count(int bit_count)
{
    return (bit_count + 8 - 1) >> 3; // 向上取整
}

void bitmap_init(bitmap_t *bitmap, u8 *bits, int count, int init_bit)
{
    bitmap->bit_count = count;
    bitmap->bits = bits;
    int bytes = bitmap_bytes_count(bitmap->bit_count);
    kernel_memset(bitmap->bits, init_bit ? 0xff : 0, bytes);
}

int bitmap_get_bit(bitmap_t *bitmap, int index)
{
    return (bitmap->bits[index >> 3] & (1 << (index % 8))) ? 1 : 0;
}

void bitamp_set_bit(bitmap_t *bitmap, int index, int count, int bit)
{
    for (int i = 0; (i < count) && (index < bitmap->bit_count); i++, index++)
    {
        if (bit)
        {
            bitmap->bits[index >> 3] |= (1 << (index % 8));
        }
        else
        {
            bitmap->bits[index >> 3] &= ~(1 << (index % 8));
        }
    }
}
// 似乎没啥用
int bitmap_is_set(bitmap_t *bitmap, int index)
{
    return bitmap_get_bit(bitmap, index) ? 1 : 0;
}

int bitmap_alloc_nbits(bitmap_t *bitmap, int bit, int count)
{
    int search_index = 0;
    int ok_index = -1;

    while (search_index < bitmap->bit_count)
    {
        if (bitmap_get_bit(bitmap, search_index) != bit)
        {
            search_index++;
            continue;
        }

        ok_index = search_index;
        int i;
        for (i = 1; (i < count) && (search_index < bitmap->bit_count); ++i)
        {
            if (bitmap_get_bit(bitmap, search_index++) != bit)
            {
                ok_index = -1;
                break;
            }
        }

        if (i >= count)
        {
            bitamp_set_bit(bitmap, ok_index, count, ~bit);
            return ok_index;
        }
    }
    return -1;
}