#include "disk.hpp"
#include "driver/console.hpp"
#include "list/klist.hpp"
#include "memory/physical_memory.hpp"
#include "memory/virtual_memory.hpp"
#include "utils.hpp"
#include <stdint.h>

UUID null_uuid = {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}};

KLIST_DEFINE(disk_list);
KLIST_DEFINE(partition_list);

int handle_mbr_disk(MBR *mbr, Disk *disk);
int handle_gpt_disk(MBR *protective_mbr, Disk *disk);

void register_disk(Disk *disk) {
	klist_add_tail(&disk_list, &disk->global);

	MBR *mbr = (MBR *)kalloc(disk->sector_size);

	disk->disk_ops.read_sectors(disk, 0, (uint8_t *)mbr, 1);

	bool is_gpt = false;
	for (uint8_t i = 0; i < 4; i++) {
		if (mbr->partitions[i].os_type == 0xEE) {
			is_gpt = true;
		}
	}

	if (is_gpt) {
		handle_gpt_disk(mbr, disk);
	} else {
		handle_mbr_disk(mbr, disk);
	}
	kfree(mbr);
}

void register_partition(Partition *partition) {
	klist_add_tail(&partition_list, &partition->global);
	printf("nr: %hhu, id: " UUID_FORMAT_STRING ", type: " UUID_FORMAT_STRING
	       ", start: 0x%lx, size: 0x%lx\n",
	       partition->number, UUID_PRINTF_ARGS(partition->id),
	       UUID_PRINTF_ARGS(partition->type), partition->volume_start,
	       partition->volume_size);
}

int handle_mbr_disk(MBR *mbr, Disk *disk) {
	printf("mbrdisk\n");

	KListHead *siblings = nullptr;

	for (uint8_t i = 0; i < 4; i++) {
		if (mbr->partitions[i].os_type == 0)
			continue;
		Partition *partition = (Partition *)kzalloc(sizeof(Partition));
		partition->number = i;
		partition->disk = disk;

		partition->type.d[7] = mbr->partitions[i].os_type;
		partition->volume_start = mbr->partitions[i].starting_LBA;
		partition->volume_size = mbr->partitions[i].size_in_LBA;
		if (!siblings) {
			klist_init(&partition->siblings);
			siblings = &partition->siblings;
		} else {
			klist_add_tail(siblings, &partition->siblings);
		}
		register_partition(partition);
	}

	return -1;
}

typedef struct GPT_partition_entry_s {
	UUID type;
	UUID part_ID;
	uint64_t starting_LBA;
	uint64_t ending_LBA;
	uint64_t Attributes;
	char partition_name[72];
} GPT_partition_entry;

typedef struct GPT_header_s {
	char signature[8];
	uint32_t revision;
	uint32_t header_size;
	uint32_t header_CRC32;
	uint32_t reserved;
	uint64_t my_lba;
	uint64_t alternate_lba;
	uint64_t first_usable_lba;
	uint64_t last_usable_lba;
	UUID disk_ID;
	uint64_t partition_entry_lba;
	uint32_t number_of_partition_entries;
	uint32_t size_of_partition_entry;
	uint32_t partition_entry_array_CRC32;
} GPT_header;

int handle_gpt_disk(MBR *protective_mbr, Disk *disk) {
	printf("gptdisk\n");
	uint8_t *buffer = (uint8_t *)kalloc(disk->sector_size);

	disk->disk_ops.read_sectors(disk, 1, buffer, 1);
	GPT_header header;
	memcpy(&header, buffer, sizeof(GPT_header));
	kfree(buffer);

	if (memcmp(header.signature, "EFI PART", 8) != 0) {
		printf("invalid gpt header\n");
		return -1;
	}
	printf(UUID_FORMAT_STRING "\n", UUID_PRINTF_ARGS(header.disk_ID));

	uint64_t partition_table_sector_count = DIV_ROUNDUP(
		header.number_of_partition_entries * header.size_of_partition_entry,
		disk->sector_size);

	uint64_t page_count =
		DIV_ROUNDUP(partition_table_sector_count * disk->sector_size, 4096);

	buffer = (uint8_t *)page_alloc(page_count);

	disk->disk_ops.read_sectors(disk, header.partition_entry_lba, buffer,
	                            partition_table_sector_count);

	KListHead *siblings = nullptr;

	for (uint64_t offset = 0, i = 0;
	     offset <
	     (header.number_of_partition_entries * header.size_of_partition_entry);
	     offset += header.size_of_partition_entry, i++) {
		GPT_partition_entry *entry = (GPT_partition_entry *)&buffer[offset];
		if (memcmp(&entry->type, &null_uuid, sizeof(UUID)) == 0) {
			continue;
		}
		Partition *partition = (Partition *)kzalloc(sizeof(Partition));
		partition->number = i;
		partition->disk = disk;
		partition->type = entry->type;
		partition->volume_start = entry->starting_LBA;
		partition->volume_size = entry->ending_LBA - entry->starting_LBA;
		partition->id = entry->part_ID;
		if (!siblings) {
			klist_init(&partition->siblings);
			siblings = &partition->siblings;
		} else {
			klist_add_tail(siblings, &partition->siblings);
		}
		register_partition(partition);
	}

	return -1;
}
