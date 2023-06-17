#ifndef DISK_H
#define DISK_H

#include "common/types.h"
#include "ipc/mutex.h"
#include "ipc/semaphore.h"

#define DISK_CNT                        2
#define DISK_NAME_SIZE                  32
#define PART_NAME_SIZE                  32
#define DISK_PARIMARY_PART_CNT          (4 + 1)
#define DISK_PER_CHANNEL                2
#define MBR_PRIMARY_PART_NR             4

#define IOBASE_PRIMARY                  0X1F0
#define DISK_DATA(disk)                 (disk->port_base + 0)
#define DISK_ERROR(disk)                (disk->port_base + 1)
#define DISK_SECTOR_COUNT(disk)         (disk->port_base + 2)
#define DISK_LBA_LO(disk)               (disk->port_base + 3)
#define DISK_LBA_MID(disk)              (disk->port_base + 4)
#define DISK_LBA_HI(disk)               (disk->port_base + 5)
#define DISK_DRIVE(disk)                (disk->port_base + 6)
#define DISK_STATUS(disk)               (disk->port_base + 7)
#define DISK_CMD(disk)                  (disk->port_base + 7)

#define DISK_CMD_READ                   0X24
#define DISK_CMD_WRITE                  0X34
#define DISK_CMD_IDENTIFY               0XEC

#define DISK_STATUS_ERR                 (1 << 0)
#define DISK_STATUS_DRQ                 (1 << 3)
#define DISK_STATUS_DF                  (1 << 5)
#define DISK_STATUS_BUSY                (1 << 7)

#define DISK_DRIVE_BASE                 0XE0

#pragma pack(1)

typedef struct _part_item_t {
    u8 boot_active;               // 分区是否活动
	u8 start_header;              // 起始header
	u16 start_sector : 6;         // 起始扇区
	u16 start_cylinder : 10;	    // 起始磁道
	u8 system_id;	                // 文件系统类型
	u8 end_header;                // 结束header
	u16 end_sector : 6;           // 结束扇区
	u16 end_cylinder : 10;        // 结束磁道
	u32 relative_sectors;	        // 相对于该驱动器开始的相对扇区数
	u32 total_sectors;            // 总的扇区数
}part_item_t;


typedef  struct _mbr_t {
	u8 code[446];                 // 引导代码区
    part_item_t part_item[MBR_PRIMARY_PART_NR];
	u8 boot_sig[2];               // 引导标志
}mbr_t;


#pragma pack()

// 描述分区
typedef struct _partinfo_t
{
    char name[PART_NAME_SIZE];
    struct _disk_t *disk;

    enum {
        FS_INVALID = 0x00,
        FS_FAT16_0 = 0X06,
        FS_FAT16_1 = 0X0E,
    }type;

    int start_sector;
    int total_sector;
}partinfo_t;


typedef struct _disk_t
{
    char name[DISK_NAME_SIZE];
    enum {
        DISK_MASTER = (0 << 4),
        DISK_SLAVE = (1 << 4),
    }drive;

    u16 port_base;
    int sector_size;
    int sector_count;
    partinfo_t partinfo[DISK_PARIMARY_PART_CNT];

    mutex_t *mutex;
    sem_t *op_sem;
}disk_t;


void disk_init (void);
void exception_handler_ide_primary(void);

#endif