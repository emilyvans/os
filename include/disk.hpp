#ifndef INCLUDE_DISK_HPP_
#define INCLUDE_DISK_HPP_
#include "driver/virtio_blk.hpp"
#include "list/klist.hpp"
#include <stdint.h>

typedef struct FileOperations_s {
	void (*write)(struct Disk_s *disk, uint64_t start_sector, void *buffer,
	              uint64_t sector_count);
	void (*read)(struct Disk_s *disk, uint64_t start_sector, void *buffer,
	             uint64_t sector_count);
} FileOperations;

typedef struct Disk_s {
	KListHead global;
	KListHead siblings;
	void *owner;
	uint64_t size_in_sectors;
	uint64_t sector_size;
	FileOperations file_ops;
} Disk;

extern KListHead disk_list;

#endif // INCLUDE_DISK_HPP_
