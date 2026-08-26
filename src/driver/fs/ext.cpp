#include "driver/fs/ext.hpp"
#include "disk.hpp"
#include "driver/console.hpp"
#include "fs/common.hpp"
#include "list/container_of.hpp"
#include "memory/virtual_memory.hpp"
#include "panic.hpp"
#include "utils.hpp"
#include <stdint.h>

typedef struct EXT2SuperBlock_s {
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
} EXT2SuperBlock;

typedef struct EXT2BlockGroupDescriptor_s {
	uint32_t block_bitmap_blockid;
	uint32_t inode_bitmap_blockid;
	uint32_t inode_table_blockid;
	uint16_t free_block_count;
	uint16_t free_inode_count;
	uint16_t used_directory_count;
	uint16_t padding;
	uint32_t reserved[3];
} EXT2BlockGroupDescriptor;

typedef struct EXT2DirectoryEntry_s {
	uint32_t inode;
	uint16_t record_length;
	uint8_t name_length;
	uint8_t file_type;
	char name[];
} EXT2DirectoryEntry;

typedef struct EXT2INode_s {
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
} EXT2INode;

typedef struct ext2PrivateData_s {
	EXT2SuperBlock *sb;
	uint32_t *inode_table_start_block;
	uint32_t inodes_per_block_group;
	uint32_t block_size;
	uint32_t inode_size;
	uint32_t block_group_count;
} ext2PrivateData;

void ext2_read_blocks(Partition *partition, uint32_t start_block, void *buffer,
                      uint32_t block_count) {
	ext2PrivateData *data = (ext2PrivateData *)partition->private_data;
	Disk *disk = partition->disk;
	void (*read)(struct Disk_s *disk, uint64_t start_sector, void *buffer,
	             uint64_t sector_count) = disk->disk_ops.read_sectors;

	int64_t block_sector_size = data->block_size / disk->sector_size;
	if (block_sector_size <= 0) {
		panic("block_size is smaller than disk sector size (%u<%lu)",
		      data->block_size, disk->sector_size);
	}
	uint64_t start_sector =
		partition->volume_start + start_block * block_sector_size;
	uint64_t sector_count = block_count * block_sector_size;
	read(disk, start_sector, buffer, sector_count);
}

EXT2INode *ext2_read_inode(uint32_t inode_nr, Partition *partition) {

	ext2PrivateData *data = (ext2PrivateData *)partition->private_data;
	EXT2INode *inode = (EXT2INode *)kzalloc(data->inode_size);
	uint32_t block_count =
		(data->inodes_per_block_group * data->inode_size) / data->block_size;
	uint8_t *buffer = (uint8_t *)kzalloc(block_count * data->block_size);

	uint32_t block_group = inode_nr / data->inodes_per_block_group;
	uint32_t inode_offset =
		((inode_nr - 1) % data->inodes_per_block_group) * data->inode_size;

	uint32_t start_block = data->inode_table_start_block[block_group];

	ext2_read_blocks(partition, start_block, buffer, block_count);

	memcpy(inode, (void *)((uint64_t)buffer + inode_offset), data->inode_size);

	kfree(buffer);
	return inode;
}

void ext2_open(const char *file_path, File *file) {

	Partition *partition = file->owner;
	ext2PrivateData *private_data = (ext2PrivateData *)partition->private_data;
	// TODO: walk dir to path
	if (*file_path == '/') {
		file_path++;
	}
	char *current_search_name = (char *)kalloc(256);
	uint8_t name_length;
	EXT2INode *current_dir_inode = ext2_read_inode(2, partition);
	while (*file_path != 0) {
		name_length = 0;

		while (*file_path != '/' && *file_path != 0) {
			current_search_name[name_length++] = *(file_path++);
		}
		current_search_name[name_length] = 0;
		if (*file_path == '/') {
			file_path++;
		}
		printf("search name: %s\n", current_search_name);

		for (uint8_t i = 0; i < 12; i++) {
			if (current_dir_inode->direct_block_id[i] == 0)
				continue;
			uint8_t *buffer = (uint8_t *)kzalloc(private_data->block_size);
			ext2_read_blocks(partition, current_dir_inode->direct_block_id[i],
			                 buffer, 1);
			for (uint64_t offset = 0; offset < private_data->block_size;) {
				EXT2DirectoryEntry *dir_entry =
					(EXT2DirectoryEntry *)(uint64_t(buffer) + offset);
				offset += dir_entry->record_length;
				if (name_length != dir_entry->name_length)
					continue;
				if (memcmp(current_search_name, dir_entry->name, name_length) ==
				    0) {
					printf("inode: %u\n", dir_entry->inode);
					kfree(current_dir_inode);
					if (dir_entry->file_type == 0x2) {
						current_dir_inode =
							ext2_read_inode(dir_entry->inode, partition);
						goto found_dir;
					} else {
						file->fs_file_id = dir_entry->inode;
						goto done;
					}
				}
			}
		}
	found_dir:
	}

done:
	kfree(current_search_name);
}

void ext2_get_dir_entry(Partition *partition, const char *name,
                        DirectoryEntry prev_dir_entry,
                        DirectoryEntry *out_dir_entry) {
	ext2PrivateData *private_data = (ext2PrivateData *)partition->private_data;
	EXT2INode *current_dir_inode;
	if (prev_dir_entry.owner == NULL && prev_dir_entry.fs_file_id == 2) {
		current_dir_inode = ext2_read_inode(2, partition);
	} else {
		current_dir_inode =
			ext2_read_inode(prev_dir_entry.fs_file_id, partition);
	}
	uint64_t name_length = strlen(name);

	for (uint8_t i = 0; i < 12; i++) {
		if (current_dir_inode->direct_block_id[i] == 0)
			continue;
		uint8_t *buffer = (uint8_t *)kzalloc(private_data->block_size);
		ext2_read_blocks(partition, current_dir_inode->direct_block_id[i],
		                 buffer, 1);
		for (uint64_t offset = 0; offset < private_data->block_size;) {
			EXT2DirectoryEntry *dir_entry =
				(EXT2DirectoryEntry *)(uint64_t(buffer) + offset);
			offset += dir_entry->record_length;
			if (name_length != dir_entry->name_length)
				continue;
			if (memcmp(name, dir_entry->name, name_length) == 0) {
				kfree(current_dir_inode);
				out_dir_entry->owner = partition;
				out_dir_entry->fs_file_id = dir_entry->inode;
				out_dir_entry->type = dir_entry->file_type;
				kfree(buffer);
				return;
			}
		}
		kfree(buffer);
	}
	out_dir_entry->owner = NULL;
	return;
};

void ext2_read_inode_blocks(Partition *partition, EXT2INode *inode,
                            uint64_t start_block, void *buffer,
                            uint64_t block_count) {
	ext2PrivateData *private_data = (ext2PrivateData *)partition->private_data;
	uint64_t i = 0;
	uint8_t *temp_buffer = (uint8_t *)kalloc(private_data->block_size);
	for (uint64_t block = start_block; block < (start_block + block_count);
	     block++) {
		if (block < 12) {
			ext2_read_blocks(partition, inode->direct_block_id[block],
			                 temp_buffer, 1);
			memcpy((uint8_t *)buffer + i * private_data->block_size,
			       temp_buffer, private_data->block_size);
		} else {
			UNIMPLEMENTED_NAME("indirect blocks");
		}
	}
	kfree(temp_buffer);
}

int ext2_read(File *file, void *buffer, uint64_t bytes) {
	ext2PrivateData *private_data =
		(ext2PrivateData *)file->owner->private_data;
	EXT2INode *file_inode = ext2_read_inode(file->fs_file_id, file->owner);
	uint8_t *file_buffer = (uint8_t *)buffer;

	uint64_t file_size =
		file_inode->high_file_size << 32 | file_inode->low_file_size;

	uint64_t bytes_to_read = bytes;

	if (file->cursor + bytes > file_size) {
		bytes_to_read = file_size - file->cursor;
	}

	uint64_t bytes_read = bytes_to_read;

	uint64_t buffer_cursor = 0;
	uint64_t prev_file_cursor = file->cursor;
	file->cursor += bytes_read;

	for (uint64_t position = prev_file_cursor;
	     position < (prev_file_cursor + bytes_to_read);) {

		uint64_t inode_block = position / private_data->block_size;
		uint64_t start_byte = position % private_data->block_size;
		uint64_t bytes_to_next_block = private_data->block_size - start_byte;

		uint8_t *disk_buffer = (uint8_t *)kzalloc(private_data->block_size);
		ext2_read_inode_blocks(file->owner, file_inode, inode_block,
		                       disk_buffer, 1);
		if (bytes_to_next_block >= bytes_to_read) {
			for (uint64_t i = 0; i < bytes_to_read; i++) {
				file_buffer[i] = disk_buffer[start_byte + i];
			}
			return bytes_read;
		}
	}

	return 0;
}

FileOperations ext2_file_ops = {
	.open = ext2_open,
	.read = ext2_read,
};

PartitionOperations ext2_part_ops = {
	.get_dir_entry = ext2_get_dir_entry,
};

void parse_ext(Partition *partition) {
	ext2PrivateData *private_data =
		(ext2PrivateData *)kalloc(sizeof(ext2PrivateData));
	partition->private_data = private_data;
	uint64_t start = partition->volume_start;
	Disk *disk = partition->disk;
	uint64_t ldas = DIV_ROUNDUP(1024, disk->sector_size);
	uint64_t byte_size = ldas * disk->sector_size;
	uint8_t *buffer = (uint8_t *)kzalloc(byte_size);

	disk->disk_ops.read_sectors(disk, start + ldas, buffer, ldas);

	printf("data (%lu Bytes):\n", byte_size);
	for (uint64_t i = 0; i < byte_size; i += 8) {
		printf("%02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx\n",
		       buffer[i], buffer[i + 1], buffer[i + 2], buffer[i + 3],
		       buffer[i + 4], buffer[i + 5], buffer[i + 6], buffer[i + 7]);
	}
	EXT2SuperBlock super_block;
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
	private_data->block_size = block_size;
	private_data->block_group_count = block_groups;

	printf("block group count: %u(%u rem %u)\n", block_groups,
	       super_block.total_blocks / super_block.num_blocks_in_block_group,
	       super_block.total_blocks % super_block.num_blocks_in_block_group);
	uint32_t bgdt_block_size = DIV_ROUNDUP(
		block_groups * sizeof(EXT2BlockGroupDescriptor), block_size);
	buffer = (uint8_t *)kzalloc(bgdt_block_size * block_size);
	ext2_read_blocks(partition, super_block.my_lba + 1, buffer,
	                 bgdt_block_size);

	EXT2BlockGroupDescriptor *block_group_descriptors =
		(EXT2BlockGroupDescriptor *)kzalloc(sizeof(EXT2BlockGroupDescriptor) *
	                                        block_groups);

	uint32_t *inode_table_start_blocks =
		(uint32_t *)kalloc(sizeof(uint32_t) * block_groups);

	for (int64_t i = 0; i < block_groups; i++) {
		EXT2BlockGroupDescriptor *bgd =
			(EXT2BlockGroupDescriptor *)(uint64_t(buffer) +
		                                 sizeof(EXT2BlockGroupDescriptor) * i);
		inode_table_start_blocks[i] = bgd->inode_table_blockid;
		memcpy(block_group_descriptors + i, bgd,
		       sizeof(EXT2BlockGroupDescriptor));
	}

	private_data->inode_table_start_block = inode_table_start_blocks;
	private_data->inodes_per_block_group =
		super_block.num_inodes_in_block_group;
	private_data->inode_size = super_block.inode_structure_size;

	for (int64_t i = 0; i < block_groups; i++) {
		EXT2BlockGroupDescriptor *bgd = block_group_descriptors + i;
		printf("block group %ld:\n block bitmap blockid: %u\n inode bitmap "
		       "blockid: %u\n inode table blockid: %u\n free block count: %u\n "
		       "free inode count: %u\n used directory count: %u\n",
		       i, bgd->block_bitmap_blockid, bgd->inode_bitmap_blockid,
		       bgd->inode_table_blockid, bgd->free_block_count,
		       bgd->free_inode_count, bgd->used_directory_count);
	}

	EXT2INode *root = ext2_read_inode(2, partition);

	uint8_t *root_inode_buf = (uint8_t *)root;

	for (uint64_t i = 0; i < super_block.inode_structure_size; i += 8) {
		printf(" %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx\n",
		       root_inode_buf[i], root_inode_buf[i + 1], root_inode_buf[i + 2],
		       root_inode_buf[i + 3], root_inode_buf[i + 4],
		       root_inode_buf[i + 5], root_inode_buf[i + 6],
		       root_inode_buf[i + 7]);
	}
	printf("mode: %#08x\n", root->flags);
	printf("blocks: %u\n", root->blocks);

	for (int64_t i = 0; i < 15; i++) {
		printf(" %ld:%u ", i, *(root->direct_block_id + i));
	}
	printf("\n");

	printf("block(%u):\n", root->direct_block_id[0]);
	uint8_t *dir_buffer = (uint8_t *)kalloc(block_size);
	ext2_read_blocks(partition, root->direct_block_id[0], dir_buffer, 1);

	for (uint64_t i = 0; i < block_size; i += 8) {
		printf(" %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx\n",
		       dir_buffer[i], dir_buffer[i + 1], dir_buffer[i + 2],
		       dir_buffer[i + 3], dir_buffer[i + 4], dir_buffer[i + 5],
		       dir_buffer[i + 6], dir_buffer[i + 7]);
	}
	printf("dir: /\n");
	char current_dir[2] = ".";
	char prev_dir[3] = "..";

	for (uint64_t offset = 0; offset < block_size;) {
		EXT2DirectoryEntry *entry =
			(EXT2DirectoryEntry *)((uint64_t)dir_buffer + offset);
		offset += entry->record_length;

		char *file_name = (char *)kzalloc(entry->name_length + 1);

		memcpy(file_name, entry->name, entry->name_length);
		if (memcmp(current_dir, file_name, 2) == 0 ||
		    memcmp(prev_dir, file_name, 3) == 0)
			continue;

		printf(" name: %s, inode: %u, record length: %hu, name length: %hhu, "
		       "file type: "
		       "%hhu\n",
		       file_name, entry->inode, entry->record_length,
		       entry->name_length, entry->file_type);
		EXT2INode *sub = ext2_read_inode(entry->inode, partition);
		printf(" ");
		for (int64_t i = 0; i < 15; i++) {
			printf(" %ld:%u ", i, *(sub->direct_block_id + i));
		}
		printf("\n");
		if (entry->file_type == 2 && entry->inode != 2) {
			for (int64_t i = 0; i < 12; i++) {
				if (sub->direct_block_id[i] == 0)
					continue;
				uint8_t *dir1_buffer = (uint8_t *)kalloc(block_size);
				ext2_read_blocks(partition, sub->direct_block_id[0],
				                 dir1_buffer, 1);
				for (uint64_t offset = 0; offset < block_size;) {
					EXT2DirectoryEntry *entry_1 =
						(EXT2DirectoryEntry *)((uint64_t)dir1_buffer + offset);
					offset += entry_1->record_length;

					char *file_name = (char *)kzalloc(entry_1->name_length + 1);

					memcpy(file_name, entry_1->name, entry_1->name_length);

					if (memcmp(current_dir, file_name, 2) == 0 ||
					    memcmp(prev_dir, file_name, 3) == 0)
						continue;

					printf("  name: %s, inode: %u, record length: %hu, "
					       "name "
					       "length: "
					       "%hhu, "
					       "file type: "
					       "%hhu\n",
					       file_name, entry_1->inode, entry_1->record_length,
					       entry_1->name_length, entry_1->file_type);
					EXT2INode *sub_1 =
						ext2_read_inode(entry_1->inode, partition);
					if (entry_1->file_type != 7) {
						printf("  ");
						for (int64_t i = 0; i < 15; i++) {
							printf(" %ld:%u ", i,
							       *(sub_1->direct_block_id + i));
						}
						printf("\n");
					} else {
						char target[61];
						memcpy(target, sub_1->direct_block_id, 60);
						printf("   target: %s\n", target);
					}

					kfree(file_name);
				}
				kfree(dir1_buffer);
			}
		} else if (entry->file_type == 1) {
			uint8_t *buf = (uint8_t *)kzalloc(block_size);
			for (uint8_t i = 0; i < 12; i++) {
				if (sub->direct_block_id[i] == 0)
					continue;
				ext2_read_blocks(partition, sub->direct_block_id[i], buf, 1);
				printf("file block %u data:\n ", i);
				for (uint64_t i = 0; i < bgdt_block_size * block_size; i += 8) {
					printf("%c%c%c%c%c%c%c"
					       "%c",
					       buf[i], buf[i + 1], buf[i + 2], buf[i + 3],
					       buf[i + 4], buf[i + 5], buf[i + 6], buf[i + 7]);
				}
				printf("\n");
			}
			kfree(buf);
		}
		kfree(sub);

		kfree(file_name);
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

FileSystemType ext2_file_system_type = {
	.file_ops = &ext2_file_ops,
	.part_ops = &ext2_part_ops,
};
