#include "fs/fs.h"
#include "common/types.h"
#include "common/CPUInlineFun.h"
#include "tools/klib.h"
#include "dev/console.h"
#include <sys/stat.h>
#include "fs/file.h"
#include "dev/dev.h"
#include "core/task.h"
#include "tools/log.h"
#include "dev/disk.h"
#include "osConfig.h"
#include "applib/lib_syscall.h"
#include <sys/file.h>

#define SECTOR_SIZE 512

#define FS_TABLE_SIZE       32

static list_t mounted_list;
static fs_t fs_table[FS_TABLE_SIZE];
static list_t free_list;

extern fs_op_t devfs_op;
extern fs_op_t fatfs_op;

static fs_t *root_fs;

static int is_path_vaild(const char *path){
    if (path == (const char *)0 || path[0] == '\0'){
        return 0;
    }

    return 1;
}

int path_to_num(const char *path, int *num){
    int n = 0;
    const char *c = path;

    while(*c){
        n = n * 10 + *c - '0';
        c++;
    }
    *num = n;
    return 0;
}
const char *path_next_child(const char *path){
    const char *c = path;
    while(*c && (*c++ == '/')) {}
    while(*c && (*c++ != '/')) {}
    
    return *c ? c : (const char *)0;
}

static void fs_protect(fs_t *fs){
    if (fs->mutex) {
        mutex_lock(fs->mutex);
    }
}

static void fs_unprotect(fs_t *fs) {
    if (fs->mutex) {
        mutex_unlock(fs->mutex);
    }
}

int path_begin_with(const char *path, const char *str) {
    const  char *s1 = path, *s2 = str;
    while(*s1 && *s2 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *s2 == '\0';
}

static int is_fd_bad(int file) {
    if ((file < 0) || (file >= TASK_OFILE_NR)) {
        return 1;
    }
    return 0;
}



int sys_open(const char *name, int flags, ...) {
	// 分配文件描述符链接
	file_t * file = file_alloc();
	if (!file) {
		return -1;
	}

	int fd = task_alloc_fd(file);
	if (fd < 0) {
		goto sys_open_failed;
	}

	// 检查名称是否以挂载点开头，如果没有，则认为name在根目录下
	// 即只允许根目录下的遍历
	fs_t *fs = (fs_t *)0;
	list_node_t * node = list_get_first(&mounted_list);
	while (node) {
		fs_t * curr = list_node_parent(node, fs_t, node);
		if (path_begin_with(name, curr->mount_point)) {
			fs = curr;
			break;
		}
		node = list_node_next(node);
	}

	if (fs) {
		name = path_next_child(name);
	} else {
		fs = root_fs;
	}

	file->mode = flags;
	file->fs = fs;
	kernel_strncpy(file->file_name, name, FILE_NAME_SIZE);

	fs_protect(fs);
	int err = fs->op->open(fs, name, file);
	if (err < 0) {
		fs_unprotect(fs);

		log_print("open %s failed.\n", name);
		return -1;
	}
	fs_unprotect(fs);

	return fd;

sys_open_failed:
	file_free(file);
	if (fd >= 0) {
		task_remove_fd(fd);
	}
	return -1;
}

int sys_read(int file, char *ptr, int len)
{
    if (is_fd_bad(file) || !ptr || !len) {
        return 0;
    }

    file_t *p_file = task_file(file);
    if (!p_file) {
        log_print("file open failed\n");
        return -1;
    }

    if (p_file->mode == O_WRONLY) {
        log_print("File is write only!\n");
        return -1;
    }

    fs_t *fs = p_file->fs;

    fs_protect(fs);
    int err = fs->op->read(ptr, len, p_file);
    fs_unprotect(fs);

    return err;
}

int sys_write(int file, char *ptr, int len){
    if (is_fd_bad(file) || !ptr || !len) {
        return 0;
    }

    file_t *p_file = task_file(file);
    if (!p_file) {
        log_print("file open failed\n");
        return -1;
    }

    if (p_file->mode == O_RDONLY) {
        log_print("File is write only!\n");
        return -1;
    }

    fs_t *fs = p_file->fs;
    
    fs_protect(fs);
    int err = fs->op->write(ptr, len, p_file);
    fs_unprotect(fs);

    return err;
}

int sys_lseek(int file, int ptr, int dir){
    if (is_fd_bad(file)) {
        return 0;
    }

    file_t *p_file = task_file(file);
    if (!p_file) {
        log_print("file open failed\n");
        return -1;
    }

    fs_t *fs = p_file->fs;
    
    fs_protect(fs);
    int err = fs->op->seek(p_file, ptr, dir);
    fs_unprotect(fs);

    return err;
}

int sys_close(int file){
    if (is_fd_bad(file)){
        log_print("file error\n");
        return -1;
    }

    file_t *p_file = task_file(file);
    if (!p_file) {
        log_print("file not opened!\n");
        return -1;
    }

    ASSERT(p_file->ref > 0);

    fs_t *fs = p_file->fs;

    if (p_file->ref-- == 1) {
        fs_protect(fs);
        fs->op->close(p_file);
        fs_unprotect(fs);

        file_free(p_file);
    }

    task_remove_fd(file);

    return 0;
}

int sys_isatty(int file)
{
    if (is_fd_bad(file)) {
        return 0;
    }

    file_t *p_file = task_file(file);
    if (!p_file) {
        log_print("file open failed\n");
        return -1;
    }

    return p_file->type == FILE_TTY;
}

int sys_fstat(int file, struct stat *st){
    if (is_fd_bad(file)) {
        return 0;
    }

    file_t *p_file = task_file(file);
    if (!p_file) {
        log_print("file open failed\n");
        return -1;
    }

    fs_t *fs = p_file->fs;

    kernel_memset(st, 0, sizeof(struct stat));

    fs_protect(fs);
    int err = fs->op->stat(p_file, st);
    fs_unprotect(fs);

    return err;
}

int sys_dup(int file){
    if (is_fd_bad(file)){
        log_print("file %d is not vaild\n", file);
        return -1;
    }

    file_t *p_file = task_file(file);
    if (!p_file){
        log_print("file not opened");
        return -1;
    }

    int fd = task_alloc_fd(p_file);
    if (fd >= 0){
        file_inc_ref(p_file);
        return fd;
    }
    log_print("no task file available!\n");
    return -1;
}


int sys_opendir(const char *name, DIR *dir) {
	fs_protect(root_fs);
	int err = root_fs->op->opendir(root_fs, name, dir);
	fs_unprotect(root_fs);
	return err;
}

int sys_readdir(DIR* dir, struct dirent *dirent) {
	fs_protect(root_fs);
	int err = root_fs->op->readdir(root_fs, dir, dirent);
	fs_unprotect(root_fs);
	return err;
}

int sys_closedir(DIR *dir) {
	fs_protect(root_fs);
	int err = root_fs->op->closedir(root_fs, dir);
	fs_unprotect(root_fs);
	return err;
}

int sys_ioctl(int fd, int cmd, int arg0, int arg1) {
	if (is_fd_bad(fd)) {
		return 0;
	}

	file_t *pfile = task_file(fd);
	if (pfile == (file_t *)0) {
		return 0;
	}

	fs_t * fs = pfile->fs;

	fs_protect(fs);
	int err = fs->op->ioctl(pfile, cmd, arg0, arg1);
	fs_unprotect(fs);
	return err;
}

int sys_unlink (const char *path) {
	fs_protect(root_fs);
	int err = root_fs->op->unlink(root_fs, path);
	fs_unprotect(root_fs);
	return err;
}

static fs_op_t *get_fs_op (fs_type_t type, int major){
    switch (type) {
    case FS_DEVFS:
        return &(devfs_op);
        break;
    case FS_FAT16:
        return &(fatfs_op);
        break;    
    default:
        return (fs_op_t *)0;
        break;
    }
}

static fs_t *mount(fs_type_t type, char *mount_point, int dev_major, int dev_minor){
    fs_t *fs = (fs_t *)0;
    log_print("mount file system, name: %s, dev: %x\n", mount_point, dev_major);

    list_node_t *curr = list_get_first(&mounted_list);
    while(curr){
        fs_t *fs = list_node_parent(curr, fs_t, node);
        if (kernel_strncmp(fs->mount_point, mount_point, FS_MOUNT_SIZE) == 0) {
            log_print("fs already mounted\n");
            goto mount_failed;
        }
        curr = list_node_next(curr);
    }

    list_node_t *free_node = list_delete_head(&free_list);
    if (!free_node){
        log_print("no free fs, mount failed\n");
        goto mount_failed;
    }

    fs = list_node_parent(free_node, fs_t, node);

    fs_op_t *op = get_fs_op(type, dev_major);
    if (!op) {
        log_print("unsupported fs type: %d", type);
        goto mount_failed;
    }

    kernel_memset(fs, 0, sizeof(fs_t));
    kernel_strncpy(fs->mount_point, mount_point, FS_MOUNT_SIZE);

    fs->op = op;

    if (op->mount(fs, dev_major, dev_minor) < 0){
        log_print("mount fs %d failed \n", mount_point);
        goto mount_failed;
    }

    list_insert_tail(&mounted_list, &fs->node);

    return fs;

mount_failed:
    if (fs) {
        list_insert_tail(&free_list, &fs->node);
    }
    return (fs_t *)0;
}

static void mount_list_init(void) {
    list_init(&free_list);
    for (int i = 0; i < FS_TABLE_SIZE; ++i){
        list_insert_head(&free_list, &fs_table[i].node);
    }
    list_init(&mounted_list);
}

void fs_init(void){
    mount_list_init();
    file_table_init();

    disk_init();
    fs_t *fs = mount(FS_DEVFS, "/dev", 0, 0);
    ASSERT(fs != (fs_t *)0);

    root_fs = mount(FS_FAT16, "/home", ROOT_DEV);
    ASSERT(root_fs != (fs_t *)0);
}