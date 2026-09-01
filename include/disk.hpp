#ifndef INCLUDE_DISK_HPP_
#define INCLUDE_DISK_HPP_
#include "fs/common.hpp"
#include "list/klist.hpp"
#include <stdint.h>

#define UUID_FORMAT_STRING                                                     \
	"%.8x-%.4hx-%.4hx-%.2hhx%.2hhx-%.2hhx%.2hhx%.2hhx%.2hhx%.2hhx%.2hhx"
#define UUID_PRINTF_ARGS(uuid)                                                 \
	uuid.a, uuid.b, uuid.c, uuid.d[0], uuid.d[1], uuid.d[2], uuid.d[3],        \
		uuid.d[4], uuid.d[5], uuid.d[6], uuid.d[7]

typedef struct DiskOperations_s {
	void (*write_sectors)(struct Disk_s *disk, uint64_t start_sector,
	                      void *buffer, uint64_t sector_count);
	void (*read_sectors)(struct Disk_s *disk, uint64_t start_sector,
	                     void *buffer, uint64_t sector_count);
	// void (*write)(struct Disk_s *disk, uint64_t start, void *buffer,
	//              uint64_t bytes);
	// void (*read)(struct Disk_s *disk, uint64_t start, void *buffer,
	//              uint64_t bytes);
} DiskOperations;

typedef struct UUID_s {
	uint32_t a;
	uint16_t b;
	uint16_t c;
	uint8_t d[8];
} UUID;

typedef struct Disk_s {
	KListHead global;
	KListHead siblings;
	KListHead partitions;
	void *owner;
	UUID id;
	uint64_t size_in_sectors;
	uint64_t sector_size;
	DiskOperations disk_ops; // TODO: make this into a operation queue
} Disk;

typedef struct Partition_s {
	KListHead global;
	KListHead siblings;
	KListHead fs_partitions;
	struct FileSystemType_s *fs;
	void *private_data;
	uint8_t number;
	Disk *disk;
	UUID id;
	UUID type;
	uint64_t volume_size;
	uint64_t volume_start;
} Partition;

typedef struct FileSystemType_s {
	KListHead global;
	KListHead partitions;
	FileOperations *file_ops;
	PartitionOperations *part_ops;
} FileSystemType;

typedef struct MBRPartitionEntry_s {
	uint8_t boot_indicator;
	uint8_t start_cylinder;
	uint8_t start_head;
	uint8_t start_sector;
	uint8_t os_type;
	uint8_t end_cylinder;
	uint8_t end_head;
	uint8_t end_sector;
	uint32_t starting_LBA;
	uint32_t size_in_LBA;
} __attribute__((packed)) MBRPartitionEntry;

typedef struct MBR_s {
	uint8_t bootstrap_code_1[218];
	uint8_t zero[2];
	uint8_t orig_phys_drive;
	uint8_t seconds;
	uint8_t minutes;
	uint8_t hours;
	uint8_t bootstrap_code_2[216];
	uint32_t disk_sig;
	uint16_t copy_protected;
	MBRPartitionEntry partitions[4];
	uint8_t boot_signature[2];
} __attribute__((packed)) MBR;

void register_disk(Disk *disk);
void register_partition(Partition partition);

extern KListHead disk_list;
extern KListHead partition_list;

#endif // INCLUDE_DISK_HPP_
