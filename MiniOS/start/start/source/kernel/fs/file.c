#include "fs/file.h"
#include "ipc/mutex.h"
#include "tools/klib.h"

static file_t file_table[FILE_TABLE_SIZE];

static mutex_t file_alloc_mutex;

void file_table_init(void){
    mutex_init(&file_alloc_mutex);
    // 这可以不要
    kernel_memset(file_table, 0, sizeof(file_table));

}

file_t *file_alloc(void){
    file_t *file = (file_t *)0;
    mutex_lock(&file_alloc_mutex);
    for (int i = 0; i < FILE_TABLE_SIZE; ++i){
        file_t *f = file_table + i;
        if (f->ref == 0){
            kernel_memset(f, 0, sizeof(f));
            f->ref = 1;
            file = f;
            break;
        }
    }
    mutex_unlock(&file_alloc_mutex);
    return file;
}

void file_free(file_t *file){
    mutex_lock(&file_alloc_mutex);
    if (file->ref){
        file->ref--;
    }
    mutex_unlock(&file_alloc_mutex);
}

void file_inc_ref(file_t *file){
    mutex_lock(&file_alloc_mutex);
    file->ref++;
    mutex_unlock(&file_alloc_mutex);
}