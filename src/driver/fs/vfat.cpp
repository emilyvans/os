#include "driver/fs/vfat.hpp"
#include "disk.hpp"
#include "driver/console.hpp"
#include "fs/common.hpp"
#include "list/container_of.hpp"
#include "memory/virtual_memory.hpp"
#include "utils.hpp"
#include <stddef.h>
#include <stdint.h>

typedef struct FAT_BPB_s {
	uint8_t jmp_boot[3];
	char oem_name[8];
	uint16_t bytes_per_sector;
	uint8_t sectors_per_cluster;
	uint16_t reserved_sector_count;
	uint8_t number_of_FATs;
	uint16_t root_entry_count;
	uint16_t total_sector_count_16;
	uint8_t media;
	uint16_t FAT_size_16;
	uint16_t sectors_per_track;
	uint16_t number_of_head;
	uint32_t hidden_sectors;
	uint32_t total_sector_count_32;
	union {
		struct {
			uint8_t drive_number;
			uint8_t reserved;
			uint8_t boot_signature;
			uint32_t volume_id;
			char volume_label[11];
			char filesystem_type[8];
		} __attribute__((packed)) FAT_12_16;
		struct {
			uint32_t FAT_size_32;
			uint16_t extra_flags;
			uint16_t filesystem_version;
			uint32_t root_cluster;
			uint16_t filesystem_info;
			uint16_t backup_boot_sector;
			uint8_t reserved[12];
			uint8_t drive_number;
			uint8_t reserved1;
			uint8_t boot_signature;
			uint32_t volume_id;
			char volume_label[11];
			char filesystem_type[8];
		} __attribute__((packed)) FAT_32;
	};
} __attribute__((packed)) FAT_BPB;

typedef struct VFATPrivateData_s {
	uint8_t fat_entry_size; //  12, 16, 32 bits
	uint64_t cluster_byte_size;
} VFATPrivateData;

void parse_vfat(Partition *partition) {
	printf("vfat partition: " UUID_FORMAT_STRING "\n",
	       UUID_PRINTF_ARGS(partition->id));

	VFATPrivateData *private_data =
		(VFATPrivateData *)kzalloc(sizeof(VFATPrivateData));

	if (sizeof(FAT_BPB) != 90) {
		printf("error fat_size: %lu must be 90", sizeof(FAT_BPB));
	}
	uint8_t *boot_sector = (uint8_t *)kalloc(512);
	Disk *disk = partition->disk;
	if (disk->disk_ops.read_sectors == nullptr) {
		printf("disk doesn't have read function");
	}

	uint64_t fat_part_start_sector = partition->volume_start;
	memset(boot_sector, 0, 512);
	disk->disk_ops.read_sectors(disk, fat_part_start_sector, boot_sector, 1);
	/*printf("bpb data: \n");
	for (uint64_t i = 0; i < 512; i += 8) {
	    printf(" 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx\n",
	           boot_sector[i + 0], boot_sector[i + 1], boot_sector[i + 2],
	           boot_sector[i + 3], boot_sector[i + 4], boot_sector[i + 5],
	           boot_sector[i + 6], boot_sector[i + 7]);
	}
	printf("\n");*/

	FAT_BPB fat_bpb;
	memcpy(&fat_bpb, boot_sector, sizeof(FAT_BPB));

	uint64_t root_directory_sectors =
		((fat_bpb.root_entry_count * 32) + (fat_bpb.bytes_per_sector - 1)) /
		fat_bpb.bytes_per_sector;

	uint32_t fat_size;
	uint32_t total_sectors;
	if (fat_bpb.FAT_size_16 != 0) {
		fat_size = fat_bpb.FAT_size_16;
	} else {
		fat_size = fat_bpb.FAT_32.FAT_size_32;
	}

	if (fat_bpb.total_sector_count_16 != 0) {
		total_sectors = fat_bpb.total_sector_count_16;
	} else {
		total_sectors = fat_bpb.total_sector_count_32;
	}
	uint64_t data_sector_count =
		total_sectors -
		(fat_bpb.reserved_sector_count + (fat_bpb.number_of_FATs * fat_size) +
	     root_directory_sectors);

	uint64_t cluster_count = data_sector_count / fat_bpb.sectors_per_cluster;

	private_data->cluster_byte_size =
		fat_bpb.bytes_per_sector * fat_bpb.sectors_per_cluster;
	printf("cluster byte size: %luB, sector byte size: %luB\n",
	       private_data->cluster_byte_size, fat_bpb.bytes_per_sector);

	printf("cluster count: %lu\n", cluster_count);
	if (cluster_count < 4085) {
		printf("FAT12\n");
		private_data->fat_entry_size = 12;
	} else if (cluster_count < 65525) {
		printf("FAT16\n");
		private_data->fat_entry_size = 16;
	} else {
		printf("FAT32\n");
		private_data->fat_entry_size = 32;
	}
	uint64_t fat_start_sector =
		fat_part_start_sector + fat_bpb.reserved_sector_count;

	disk->disk_ops.read_sectors(disk, fat_start_sector, boot_sector, 1);
	printf("fat data(part_start + %hu): \n", fat_bpb.reserved_sector_count);
	for (uint64_t i = 0; i < 512; i += 8) {
		printf(" 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx\n",
		       boot_sector[i + 0], boot_sector[i + 1], boot_sector[i + 2],
		       boot_sector[i + 3], boot_sector[i + 4], boot_sector[i + 5],
		       boot_sector[i + 6], boot_sector[i + 7]);
	}
	printf("\n");
	return;
	for (uint64_t sector = fat_part_start_sector +
	                       fat_bpb.reserved_sector_count +
	                       (fat_bpb.number_of_FATs * fat_size),
	              i = 0;
	     i < root_directory_sectors; i++, sector++) {
		disk->disk_ops.read_sectors(disk, sector, boot_sector, 1);

		printf("root dir(%lu sec): \n", sector - fat_part_start_sector);
		for (uint64_t i = 0; i < 512; i += 8) {
			printf(" 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx\n",
			       boot_sector[i + 0], boot_sector[i + 1], boot_sector[i + 2],
			       boot_sector[i + 3], boot_sector[i + 4], boot_sector[i + 5],
			       boot_sector[i + 6], boot_sector[i + 7]);
		}
		printf("\n");
	}
}

void vfat_get_dir_entry(struct Partition_s *partition, const char *name,
                        DirectoryEntry prev_dir_entry,
                        DirectoryEntry *out_dir_entry) {}

int vfat_read(File *file, void *buffer, uint64_t bytes) {}

FileOperations_s vfat_file_ops = {
	.open = NULL,
	.read = vfat_read,
};

PartitionOperations_s vfat_part_ops = {
	.get_dir_entry = vfat_get_dir_entry,
};

FileSystemType vfat_file_system_type = {
	.file_ops = &vfat_file_ops,
	.part_ops = &vfat_part_ops,
};
