#ifndef FAT16_H
#define FAT16_H

#include "common/types.h"
#include "ipc/mutex.h"

#pragma pack(1)

#define FAT_CLUSTER_INVALID 		    0xFFF8      	    // 无效的簇号

#define DIRITEM_NAME_FREE               0xE5                // 目录项空闲名标记
#define DIRITEM_NAME_END                0x00                // 目录项结束名标记

#define DIRITEM_ATTR_READ_ONLY          0x01                // 目录项属性：只读
#define DIRITEM_ATTR_HIDDEN             0x02                // 目录项属性：隐藏
#define DIRITEM_ATTR_SYSTEM             0x04                // 目录项属性：系统类型
#define DIRITEM_ATTR_VOLUME_ID          0x08                // 目录项属性：卷id
#define DIRITEM_ATTR_DIRECTORY          0x10                // 目录项属性：目录
#define DIRITEM_ATTR_ARCHIVE            0x20                // 目录项属性：归档
#define DIRITEM_ATTR_LONG_NAME          0x0F                // 目录项属性：长文件名

#define SFN_LEN                    	 	11              // sfn文件名长

#define FAT_CLUSTER_FREE                0x00

typedef struct _diritem_t {
    u8 DIR_Name[11];                   // 文件名
    u8 DIR_Attr;                      // 属性
    u8 DIR_NTRes;                      // 保留
    u8 DIR_CrtTimeTeenth;             // 创建时间的毫秒
    u16 DIR_CrtTime;         // 创建时间
    u16 DIR_CrtDate;         // 创建日期
    u16 DIR_LastAccDate;     // 最后访问日期
    u16 DIR_FstClusHI;                // 簇号高16位
    u16 DIR_WrtTime;         // 修改时间
    u16 DIR_WrtDate;         // 修改时期
    u16 DIR_FstClusL0;                // 簇号低16位
    u32 DIR_FileSize;                 // 文件字节大小
} diritem_t;

typedef struct _dbr_t
{
    u8 BS_jmpBoot[3];                 // 跳转代码
    u8 BS_OEMName[8];                 // OEM名称
    u16 BPB_BytsPerSec;               // 每扇区字节数
    u8 BPB_SecPerClus;                // 每簇扇区数
    u16 BPB_RsvdSecCnt;               // 保留区扇区数
    u8 BPB_NumFATs;                   // FAT表项数
    u16 BPB_RootEntCnt;               // 根目录项目数
    u16 BPB_TotSec16;                 // 总的扇区数
    u8 BPB_Media;                     // 媒体类型
    u16 BPB_FATSz16;                  // FAT表项大小
    u16 BPB_SecPerTrk;                // 每磁道扇区数
    u16 BPB_NumHeads;                 // 磁头数 
    u32 BPB_HiddSec;                  // 隐藏扇区数
    u32 BPB_TotSec32;                 // 总的扇区数

	u8 BS_DrvNum;                     // 磁盘驱动器参数
	u8 BS_Reserved1;				  // 保留字节
	u8 BS_BootSig;                    // 扩展引导标记
	u32 BS_VolID;                     // 卷标序号
	u8 BS_VolLab[11];                 // 磁盘卷标
	u8 BS_FileSysType[8];             // 文件类型名称
}dbr_t;

#pragma pack()

typedef struct _fat_t
{
    u32 tbl_start;                     // FAT表起始扇区号
    u32 tbl_cnt;                       // FAT表数量
    u32 tbl_sectors;                   // 每个FAT表的扇区数
    u32 bytes_per_sec;                 // 每扇区大小
    u32 sec_per_cluster;               // 每簇的扇区数
    u32 root_ent_cnt;                  // 根目录的项数
    u32 root_start;                    // 根目录起始扇区号
    u32 data_start;                    // 数据区起始扇区号
    u32 cluster_byte_size;             // 每簇字节数
    u32 curr_sector;                    // 扇区缓存

    u8 *fat_buffer;                    // FAT表项缓冲

    struct _fs_t *fs;                  // 所在的文件系统

    mutex_t mutex;
}fat_t;

typedef u16 cluster_t;



#endif