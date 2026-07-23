#include "driver/fs/ext.hpp"
#include "disk.hpp"
#include "driver/console.hpp"
#include "memory/virtual_memory.hpp"
#include "panic.hpp"
#include "utils.hpp"
#include <stdint.h>

typedef struct SuperBlock_s {
	uint32_t total_inodes;
	uint32_t total_blocks;
	uint32_t reserved_blocks;
	uint32_t unalloced_blocks;
	uint32_t unalloced_inodes;
	uint32_t my_lba;
	uint32_t block_size;    // actual block size = 1024 << block_size
	uint32_t fragment_size; // actual fragment size = 1024 << fragment_size
	uint32_t num_blocks_in_block_group;
	uint32_t num_fragments_in_block_group;
	uint32_t num_inodes_in_block_group;
	uint32_t last_mount_time;
	uint32_t last_written_time;
	uint16_t num_mounted_since_checked;
	uint16_t num_mounted_before_check;
	uint16_t signature;
	uint16_t file_system_state;
	uint16_t error_handling;
	uint16_t minor_version;
	uint32_t posix_time_last_check;
	uint32_t posix_time_interval_forced_check;
	uint32_t os_id;
	uint32_t major_version;
	uint16_t reserved_user_id;
	uint16_t reserved_group_id;

	uint32_t first_non_reseved_inode;
	uint16_t inode_structure_size; // in bytes
	uint16_t my_block_group;
	uint32_t optional_features;
	uint32_t required_features;
	uint32_t read_only_features;
	UUID file_system_id;
	char volume_name[16];
	char last_mounted_path[64];
	uint32_t compression_algorithm;
	uint8_t blocks_to_preallocate_files;
	uint8_t blocks_to_preallocate_dirs;
	uint16_t unused;
	UUID journal_id;
	uint32_t journal_inode;
	uint32_t journal_device;
	uint32_t head_orphan_inode;
} SuperBlock;

typedef struct BlockGroupDescriptor_s {
	uint32_t block_bitmap_blockid;
	uint32_t inode_bitmap_blockid;
	uint32_t inode_table_blockid;
	uint16_t free_block_count;
	uint16_t free_inode_count;
	uint16_t used_directory_count;
	uint16_t padding;
	uint32_t reserved[3];
} BlockGroupDescriptor;

typedef struct DirectoryEntry_s {
	uint32_t inode;
	uint16_t record_length;
	uint8_t name_length;
	uint8_t file_type;
	char name[];
} DirectoryEntry;

typedef struct INode_s {
	uint16_t mode;
	uint16_t user_id;
	union {
		uint32_t file_size;
		uint32_t low_file_size;
	};
	uint32_t accessed_time;
	uint32_t created_time;
	uint32_t modified_time;
	uint32_t deleted_time;
	uint16_t group_id;
	uint16_t links_count;
	uint32_t blocks;
	uint32_t flags;
	uint32_t os_specific1;
	uint32_t direct_block_id[12];
	uint32_t indirect_block_id;
	uint32_t double_indirect_block_id;
	uint32_t triple_indirect_block_id;
	uint32_t generation;
	uint32_t file_acl;
	union {
		uint32_t directory_acl;
		uint32_t high_file_size;
	};
	uint32_t file_fragment_address;
	uint32_t os_specific2[3];
} INode;

void read_block(Partition *partition, uint32_t block_size, uint32_t start_block,
                uint8_t *buffer, uint32_t block_count) {
	Disk *disk = partition->disk;
	void (*read)(struct Disk_s *disk, uint64_t start_sector, void *buffer,
	             uint64_t sector_count) = disk->disk_ops.read;

	int64_t block_sector_size = block_size / disk->sector_size;
	if (block_sector_size <= 0) {
		panic("block_size is smaller than disk sector size (%u<%lu)",
		      block_size, disk->sector_size);
	}
	uint64_t start_sector =
		partition->volume_start + start_block * block_sector_size;
	uint64_t sector_count = block_count * block_sector_size;
	read(disk, start_sector, buffer, sector_count);
}

void parse_ext(Partition *partition) {
	uint64_t start = partition->volume_start;
	Disk *disk = partition->disk;
	uint64_t ldas = DIV_ROUNDUP(1024, disk->sector_size);
	uint64_t byte_size = ldas * disk->sector_size;
	uint8_t *buffer = (uint8_t *)kzalloc(byte_size);

	disk->disk_ops.read(disk, start + ldas, buffer, ldas);

	printf("data (%lu Bytes):\n", byte_size);
	for (uint64_t i = 0; i < byte_size; i += 8) {
		printf("%02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx\n",
		       buffer[i], buffer[i + 1], buffer[i + 2], buffer[i + 3],
		       buffer[i + 4], buffer[i + 5], buffer[i + 6], buffer[i + 7]);
	}
	SuperBlock super_block;
	memcpy(&super_block, buffer, sizeof(super_block));
	kfree(buffer);
	printf(
		"superblock:\n total_inodes=%u\n total_blocks=%u\n "
		"reserved_blocks=%u\n "
		"unalloced_blocks=%u\n unalloced_inodes=%u\n my_lba=%u\n "
		"block_size=%u\n fragment_size=%u\n "
		"num_blocks_in_block_group=%u\n num_fragments_in_block_group=%u\n "
		"num_inodes_in_block_group=%u\n last_mount_time=%u\n "
		"last_written_time=%u\n "
		"num_mounted_since_checked=%hu\n num_mounted_before_check=%hu\n "
		"signature=%#hx\n file_system_state=%hu\n error_handling=%hu\n "
		"minor_version=%hu\n "
		"posix_time_last_check=%u\n posix_time_interval_forced_check=%u\n "
		"os_id=%u\n major_version=%u\n reserved_user_id=%hu\n "
		"reserved_group_id=%hu\n "
		"first_non_reseved_inode=%u\n inode_structure_size=%hu\n "
		"my_block_group=%b\n optional_features=%#b\n required_features=%#b\n "
		"read_only_features=%#b\n "
		"file_system_id=" UUID_FORMAT_STRING
		"\n volume_name=\"%.16s\"\n last_mounted_path=\"%.64s\"\n "
		"compression_algorithm=%u\n "
		"blocks_to_preallocate_files=%u\n blocks_to_preallocate_dirs=%u\n "
		"journal_id=" UUID_FORMAT_STRING
		"\n journal_inode=%u\n journal_device=%u\n "
		"head_orphan_inode=%u\n",
		super_block.total_inodes, super_block.total_blocks,
		super_block.reserved_blocks, super_block.unalloced_blocks,
		super_block.unalloced_inodes, super_block.my_lba,
		1024 << super_block.block_size, 1024 << super_block.fragment_size,
		super_block.num_blocks_in_block_group,
		super_block.num_fragments_in_block_group,
		super_block.num_inodes_in_block_group, super_block.last_mount_time,
		super_block.last_written_time, super_block.num_mounted_since_checked,
		super_block.num_mounted_before_check, super_block.signature,
		super_block.file_system_state, super_block.error_handling,
		super_block.minor_version, super_block.posix_time_last_check,
		super_block.posix_time_interval_forced_check, super_block.os_id,
		super_block.major_version, super_block.reserved_user_id,
		super_block.reserved_group_id, super_block.first_non_reseved_inode,
		super_block.inode_structure_size, super_block.my_block_group,
		super_block.optional_features, super_block.required_features,
		super_block.read_only_features,
		UUID_PRINTF_ARGS(super_block.file_system_id), super_block.volume_name,
		super_block.last_mounted_path, super_block.compression_algorithm,
		(unsigned)super_block.blocks_to_preallocate_files,
		(unsigned)super_block.blocks_to_preallocate_dirs,
		UUID_PRINTF_ARGS(super_block.journal_id), super_block.journal_inode,
		super_block.journal_device, super_block.head_orphan_inode);

	uint32_t block_groups = DIV_ROUNDUP(super_block.total_blocks,
	                                    super_block.num_blocks_in_block_group);
	uint32_t block_size = 1024 << super_block.block_size;

	printf("block group count: %u(%u rem %u)\n", block_groups,
	       super_block.total_blocks / super_block.num_blocks_in_block_group,
	       super_block.total_blocks % super_block.num_blocks_in_block_group);
	uint32_t bgdt_block_size =
		DIV_ROUNDUP(block_groups * sizeof(BlockGroupDescriptor), block_size);
	buffer = (uint8_t *)kzalloc(bgdt_block_size * block_size);
	read_block(partition, block_size, super_block.my_lba + 1, buffer,
	           bgdt_block_size);

	BlockGroupDescriptor *block_group_descriptors =
		(BlockGroupDescriptor *)kzalloc(sizeof(BlockGroupDescriptor) *
	                                    block_groups);

	for (int64_t i = 0; i < block_groups; i++) {
		BlockGroupDescriptor *bgd =
			(BlockGroupDescriptor *)(uint64_t(buffer) +
		                             sizeof(BlockGroupDescriptor) * i);
		memcpy(block_group_descriptors + i, bgd, sizeof(BlockGroupDescriptor));
	}

	for (int64_t i = 0; i < block_groups; i++) {
		BlockGroupDescriptor *bgd = block_group_descriptors + i;
		printf("block group %ld:\n block bitmap blockid: %u\n inode bitmap "
		       "blockid: %u\n inode table blockid: %u\n free block count: %u\n "
		       "free inode count: %u\n used directory count: %u\n",
		       i, bgd->block_bitmap_blockid, bgd->inode_bitmap_blockid,
		       bgd->inode_table_blockid, bgd->free_block_count,
		       bgd->free_inode_count, bgd->used_directory_count);
	}

#if 0
	printf("block %u data:\n", super_block.my_lba + 1);
	for (uint64_t i = 0; i < bgdt_block_size * block_size; i += 8) {
		printf(" %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx\n",
		       buffer[i], buffer[i + 1], buffer[i + 2], buffer[i + 3],
		       buffer[i + 4], buffer[i + 5], buffer[i + 6], buffer[i + 7]);
	}
#endif
}
