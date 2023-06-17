#include "dev/disk.h"
#include "tools/log.h"
#include "tools/klib.h"
#include "common/CPUInlineFun.h"
#include "common/boot_info.h"
#include "dev/dev.h"
#include "cpu/irq.h"

static mutex_t mutex;
static sem_t op_sem;

static int irq_flags_for_diskio;

static disk_t disk_buf[DISK_CNT];

static void disk_send_cmd(disk_t *disk, u32 start_sector, u32 sector_count, int cmd) {
    outb(DISK_DRIVE(disk), DISK_DRIVE_BASE | disk->drive);

    outb(DISK_SECTOR_COUNT(disk), (u8)(sector_count >> 8));
    outb(DISK_LBA_LO(disk), (u8)(start_sector >> 24));
    outb(DISK_LBA_MID(disk), 0);
    outb(DISK_LBA_HI(disk), 0);

    outb(DISK_SECTOR_COUNT(disk), (u8)(sector_count));
    outb(DISK_LBA_LO(disk), (u8)(start_sector >> 0));
    outb(DISK_LBA_MID(disk), (u8)(start_sector >> 8));
    outb(DISK_LBA_HI(disk), (u8)(start_sector >> 16));

    outb(DISK_CMD(disk), (u8)cmd);
}

static void disk_read_data(disk_t *disk, void *buf, int size) {
    u16 *c = (u16 *)buf;

    for(int i = 0; i < size / 2; ++i) {
        *c++ = inw(DISK_DATA(disk));
    }
}

static void disk_write_data(disk_t *disk, void *buf, int size) {
    u16 *c = (u16 *)buf;

    for(int i = 0; i < size / 2; i++) {
        outw(DISK_DATA(disk), *c++);
    }
}

static int disk_wait_data(disk_t *disk) {
    u8 status;
    do {
        status = inb(DISK_STATUS(disk));
        if((status & (DISK_STATUS_BUSY | DISK_STATUS_DRQ | DISK_STATUS_ERR)) != DISK_STATUS_BUSY) {
            break;
        }
    }while (1);
    return (status & DISK_STATUS_ERR) ? -1 : 0;
}

static void print_disk_info(disk_t *disk) {
    log_print("%s", disk->name);
    log_print("     port base: %x", disk->port_base);
    log_print("     total size: %d m",disk->sector_count * disk->sector_size / 1024 / 1024);
    log_print("     drive: %s\n", disk->drive == DISK_MASTER ? "Master" : "Slave");

    for (int i = 0; i < DISK_PARIMARY_PART_CNT; ++i) {
        partinfo_t *part = disk->partinfo + i;
        if (part->type != FS_INVALID) {
            log_print("%s: type: %x, start sector: %d, count: %d\n", 
            part->name, part->type, part->start_sector, part->total_sector);
        }
    }
}

static int detect_part_info(disk_t *disk) {
    mbr_t mbr;

    disk_send_cmd(disk, 0, 1, DISK_CMD_READ);
    int err = disk_wait_data(disk);
    if(err < 0) {
        log_print("read mbr failed\n");
        return err;
    }

    disk_read_data(disk, &mbr, sizeof(mbr));
    part_item_t *item = mbr.part_item;
    partinfo_t *part_info = disk->partinfo + 1;
    for (int i = 0; i < MBR_PRIMARY_PART_NR; i++, item++, part_info++) {
        part_info->type = item->system_id;
        if (part_info->type == FS_INVALID) {
            part_info->total_sector = 0;
            part_info->start_sector = 0;
            part_info->disk = (disk_t *)0;
        } else {
            kernel_sprintf(part_info->name, "%s%d", disk->name, i + 1);
            part_info->start_sector = item->relative_sectors;
            part_info->total_sector = item->total_sectors;
            part_info->disk = disk;
        }
    }
}

static int identify_disk (disk_t *disk) {
    disk_send_cmd(disk, 0, 0, DISK_CMD_IDENTIFY);

    int err = inb(DISK_STATUS(disk));
    if (err == 0) {
        log_print("%s do not exist\n", disk->name);
    }

    err = disk_wait_data(disk);
    if (err < 0) {
        log_print("disk[%s]: read failed\n", disk->name);
        return err;
    }

    u16 buf[256];
    disk_read_data(disk, buf, sizeof(buf));
    disk->sector_count = *(u32 *)(buf + 100);
    disk->sector_size = SECTOR_SIZE;


    partinfo_t *part = disk->partinfo;
    part->disk = disk;
    kernel_sprintf(part->name, "%s%d", disk->name, 0);
    part->start_sector = 0;
    part->total_sector = disk->sector_count;
    part->type = FS_INVALID;

    detect_part_info(disk);

    return 0;
}

void disk_init(void) {
    log_print("Check disk......\n");

    kernel_memset(disk_buf, 0, sizeof(disk_buf));

    mutex_init(&mutex);
    sem_init(&op_sem, 0);

    for (int i = 0; i < DISK_CNT; ++i) {
        disk_t *disk = disk_buf + i;
        kernel_sprintf(disk->name, "sd%c", i + 'a');
        disk->drive = (i == 0) ? DISK_MASTER : DISK_SLAVE;
        disk->port_base = IOBASE_PRIMARY;
        disk->mutex = &mutex;
        disk->op_sem = &op_sem;

        int err = identify_disk(disk);
        if (err == 0) {
            print_disk_info(disk);
        }
    }
}

int disk_open(device_t *dev) {
    int disk_index = (dev->minor >> 4) - 0xa;
    int part_index = dev->minor & 0xf;

    if ((disk_index >= DISK_CNT) || (part_index >= DISK_PARIMARY_PART_CNT)) {
        log_print("device minor error: %d\n", dev->minor);
        return -1;
    }

    disk_t *disk = disk_buf + disk_index;
    if (disk->sector_count == 0) {
        log_print("disk not exist, device: sd%x\n", dev->minor);
        return -1;
    }

    partinfo_t *part_info = disk->partinfo + part_index;
    if (part_info->total_sector == 0) {
        log_print("part not exist, device: sd%x\n",dev->minor);
        return - 1;
    }

    dev->data = part_info;

    irq_install(IRQ14_HADRDISK_PRIMARY, (irq_handler_t)exception_handler_ide_primary);
    irq_enable(IRQ14_HADRDISK_PRIMARY);


    return 0;
}

// addr:磁盘扇区号， size:要读取的扇区数量
int disk_read(device_t *dev, int addr, char *buf, int size){
    partinfo_t *part = (partinfo_t *)dev->data;
    if (!part) {
        log_print("Get part info failed, device: %d\n", dev->minor);
        return -1;
    }

    disk_t *disk = part->disk;
    if (disk == (disk_t *)0) {
        log_print("No disk, device: %d\n", dev->minor);
        return -1;
    }

    mutex_lock(disk->mutex);
    irq_flags_for_diskio = 1;

    disk_send_cmd(disk, part->start_sector + addr, size, DISK_CMD_READ);
    int cnt;
    for (cnt = 0; cnt < size; cnt++, buf += disk->sector_size) {
        if(task_get_current_task()) {
            sem_wait(disk->op_sem);
        }

        int err = disk_wait_data(disk);
        if (err < 0) {
            log_print("disk(%s), read error: start sector %d,count: %d", disk->name, addr, size);
            break;
        }
        disk_read_data(disk, buf, disk->sector_size);
    }
    mutex_unlock(disk->mutex);
    return cnt;
}

int disk_write (device_t * dev, int start_sector, char * buf, int count) {
    // 取分区信息
    partinfo_t * part_info = (partinfo_t *)dev->data;
    if (!part_info) {
        log_print("Get part info failed! device = %d\n", dev->minor);
        return -1;
    }

    disk_t * disk = part_info->disk;
    if (disk == (disk_t *)0) {
        log_print("No disk for device %d\n", dev->minor);
        return -1;
    }

    mutex_lock(disk->mutex);
    irq_flags_for_diskio = 1;

    int cnt;
    disk_send_cmd(disk, part_info->start_sector + start_sector, count, DISK_CMD_WRITE);
    for (cnt = 0; cnt < count; cnt++, buf += disk->sector_size) {
        // 先写数据
        disk_write_data(disk, buf, disk->sector_size);

        // 利用信号量等待中断通知，等待写完成
        if (task_get_current_task()) {
            sem_wait(disk->op_sem);
        }

        // 这里虽然有调用等待，但是由于已经是操作完毕，所以并不会等
        int err = disk_wait_data(disk);
        if (err < 0) {
            log_print("disk(%s) write error: start sect %d, count %d\n", disk->name, start_sector, count);
            break;
        }
    }

    mutex_unlock(disk->mutex);
    return cnt;
}

int disk_control(device_t *dev, int cmd, int arg0, int arg1){
    return -1;
}

int disk_close(device_t *dev){

}

void do_handler_ide_primary(exception_frame_t *frame){
    irq_send_eoi(IRQ14_HADRDISK_PRIMARY);
    if (irq_flags_for_diskio && task_get_current_task()){
        sem_post(&op_sem);
    }
}

dev_desc_t dev_disk_desc = {
    .name = "disk",
    .major = DEV_DISK,
    .open = disk_open,
    .read = disk_read,
    .write = disk_write,
    .control = disk_control,
    .close = disk_close,
};