#ifndef INCLUDE_FS_COMMON_HPP_
#define INCLUDE_FS_COMMON_HPP_

#include <stdint.h>

typedef struct File_s {
	struct Partition_s *owner;
	uint64_t fs_file_id;
	uint64_t cursor;
} File;

typedef struct DirectoryEntry_s {
	struct Partition_s *owner;
	uint32_t fs_file_id;
	uint8_t type;
} DirectoryEntry;

typedef struct PartitionOperations_s {
	void (*get_dir_entry)(struct Partition_s *partition, const char *name,
	                      DirectoryEntry prev_dir_entry,
	                      DirectoryEntry *out_dir_entry);
} PartitionOperations;

typedef struct FileOperations_s {
	void (*open)(const char *file_path, File *file);
	int (*read)(File *file, void *buffer, uint64_t bytes);
} FileOperations;

#endif // INCLUDE_FS_COMMON_HPP_
