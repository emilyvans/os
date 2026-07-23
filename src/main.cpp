#include "cpu/GDT.hpp"
#include "cpu/asm.hpp"
#include "cpu/interrupts.hpp"
#include "disk.hpp"
#include "driver/acpi.hpp"
#include "driver/console.hpp"
#include "driver/device.hpp"
#include "driver/fs/ext.hpp"
#include "driver/init.hpp"
#include "driver/keyboard/keyboard.hpp"
#include "driver/pci.hpp"
#include "driver/pic.hpp"
#include "driver/uart.hpp"
#include "driver/virtio_blk.hpp"
#include "limine/limine_requests.hpp"
#include "list/container_of.hpp"
#include "list/klist.hpp"
#include "memory/physical_memory.hpp"
#include "memory/virtual_memory.hpp"
#include "panic.hpp"
#include "stdbool.h"
#include "stddef.h"
#include "utils.hpp"
#include <limine.h>
#include <stddef.h>
#include <stdint.h>

#define background_color 0x1d1f21

// Set the base revision to 3, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.

__attribute__((used, section(".limine_requests"))) static volatile uint64_t
	limine_base_revision[] = LIMINE_BASE_REVISION(4);

// Finally, define the start and end markers for the Limine requests.
// These can also be moved anywhere, to any .c file, as seen fit.
__attribute__((
	used,
	section(".limine_requests_"
            "start"))) static volatile uint64_t limine_requests_start_marker[] =
	LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end"))) static volatile uint64_t
	limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

void pf_handler(InterruptFrame *frame) {
	uint64_t fault_address;

	// CR2 contains the linear address that caused the fault
	asm volatile("mov %%cr2, %0" : "=r"(fault_address));
	printf("\npage fault\ntype: %x\nip: %lx\ncs: %lx\nflags: %lx\nsp: %lx\nss: "
	       "%lx\nerror code: %lx\noffending address: 0x%lx",
	       0xE, frame->rip, frame->cs, frame->flags, frame->sp, frame->ss,
	       frame->error_code, fault_address);
	hcf();
}
void double_fault_handler(InterruptFrame *frame) {
	(void)frame; // to remove warning
	clear_console();
	printf("double fault\n");
	hcf();
}
void gp_handler(InterruptFrame *frame) {
	// clear_console();
	printf("general protation\nRIP=%#zx CS=%#zx RFLAGS=%#zx\nerror=%lb\n",
	       frame->rip, frame->cs, frame->flags, frame->error_code);
	printf(
		"InterruptFrame: "
		"r15=%#zx\nr14=%#zx\nr13=%#zx\nr12=%#zx\nr11=%#zx\nr10=%#zx\nr9=%#"
		"zx\nr8=%#zx\n"
		"rbp=%#zx\nrdi=%#zx\nrsi=%#zx\nrdx=%#zx\nrcx=%#zx\nrbx=%#zx\nrax=%#zx\n"
		"interrupt_nr=%#zx\nsp=%#zx\n"
		"ss=%#zx\n",
		frame->r15, frame->r14, frame->r13, frame->r12, frame->r11, frame->r10,
		frame->r9, frame->r8, frame->rbp, frame->rdi, frame->rsi, frame->rdx,
		frame->rcx, frame->rbx, frame->rax, frame->interupt_nr, frame->sp,
		frame->ss

	);
	hcf();
}
void iop_handler(InterruptFrame *frame) {
	(void)frame; // to remove warning
	clear_console();
	printf("invalid opcode\n");
	printf("RIP: 0x%lx\n", frame->rip);
	printf("opcode byte 1: 0x%x", *(uint8_t *)frame->rip);
	hcf();
}

volatile uint64_t miliseconds = 0;

void PIC_timer_handler() {
	// printf("timer\n");
	miliseconds += 1;
}

void keyboard_interrupt_handler() {
	ps2_on_interrupt();
}

void PCI_interrupt_handler(InterruptFrame *frame) {
	clear_console();
	printf("pci int\n");
}

void unknown_handler(InterruptFrame *frame) {
	clear_console();
	if (frame->interupt_nr > 0x20 && frame->interupt_nr < 0x30) {
		printf("unknow pic interrupt\noriginal number: %lx\npic number: "
		       "%lx(%lu)\n",
		       frame->interupt_nr, frame->interupt_nr - 0x20,
		       frame->interupt_nr - 0x20);
	} else {
		printf("unknow exception\nnumber: %lx\n", frame->interupt_nr);
	}
	hcf();
}

void interrupt_handler(InterruptFrame *frame) {

	if (frame->interupt_nr == 0x27) {
		uint8_t isr = PIC_get_master_isr();
		if (!(isr & (1 << 7))) {
			return;
		}
	} else if (frame->interupt_nr == 0x2F) {
		uint8_t isr = PIC_get_slave_isr();
		if (!(isr & (1 << 7))) {
			PIC_send_master_EOI();
			return;
		}
	}

	switch (frame->interupt_nr) {
	case 0x6:
		iop_handler(frame);
		break;
	case 0x8:
		double_fault_handler(frame);
		break;
	case 0xD:
		gp_handler(frame);
		break;
	case 0xE:
		pf_handler(frame);
		break;
	case 0x20:
		PIC_timer_handler();
		PIC_send_EOI(0x0);
		break;
	case 0x21:
		keyboard_interrupt_handler();
		PIC_send_EOI(0x1);
		break;
	case 0x2B:
		PCI_interrupt_handler(frame);
		PIC_send_EOI(0xB);
		break;
	default:
		unknown_handler(frame);
	}
}

void sleep(uint64_t ms) {

	uint64_t start_time = miliseconds;

	while ((miliseconds - start_time) < ms) {
		asm volatile("hlt");
	}

	return;
}

/// FIXME: temp
void test();
/// end temp

extern "C" void kmain(void) {
	if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
		hcf();
	}

	if (framebuffer_request.response == NULL ||
	    framebuffer_request.response->framebuffer_count < 1) {
		hcf();
	}

	if (hhdm_request.response == NULL) {
		hcf();
	}

	limine_framebuffer *framebuffer =
		framebuffer_request.response->framebuffers[0];

	uint32_t scale = 2;

	init_display(framebuffer, background_color, scale);
	uart_init();

	init_GDT();
	init_IDT();
	init_PIC();

	physicalmemory::initialize();
	virtualmemory::initialize();

// ps2_keyboard_get_current_keyset();
// ps2_flush_keycode_buffer();
#if 1
	register_bus(&pci_bus);
	init_ACPI();
#else
	for (uint8_t i = 0; i < 32; i++) {
		uint32_t vender_device_id = pci_config_read(0, i, 0, 0);
		if ((vender_device_id & 0xFFFF) != 0xFFFF) {
			printf("%u: vendor: %x, device: %x\n", i, vender_device_id & 0xFFFF,
			       (vender_device_id & 0xFFFF0000) >> 16);
		}
	}
#endif

	// init_shell();
// Programmable interrupt timer setup
#define PIT_CHANNEL_0 0x40
#define PIT_CHANNEL_1 0x41
#define PIT_CHANNEL_2 0x42
#define PIT_MODE_COMMAND_REGISTER 0x43
	outb(PIT_MODE_COMMAND_REGISTER, 0b00110100);

	uint64_t PIT_FREQ = 1193182; // 1.193182MHZ
	uint16_t freq = 1000;
	uint16_t freq_divider = PIT_FREQ / freq;
	outb(PIT_CHANNEL_0, freq_divider & 0xFF);
	outb(PIT_CHANNEL_0, (freq_divider & 0xFF00) >> 8);

	PIC_unmask_interrupt(0);
	// while (miliseconds < 10000) {
	// };

	// PIC_unmask_interrupt(11);

	clear_console();
	printf("total Memory: %luMiB\nfree memory:  %luMiB\n",
	       physicalmemory::get_total_ram() / 1024 / 1024,
	       physicalmemory::get_free_ram() / 1024 / 1024);
	register_pci_driver(&virtio_blk_drv);

	printf(
		"disk: " UUID_FORMAT_STRING ", part:" UUID_FORMAT_STRING "\n\n",
		UUID_PRINTF_ARGS(
			executable_file_request.response->executable_file->gpt_disk_uuid),
		UUID_PRINTF_ARGS(
			executable_file_request.response->executable_file->gpt_part_uuid));

	test();

	//  convert this to a non busy-loop
	for (;;) {
		// void *mem = page_alloc(255);
		// printf("total Memory: %uMiB\nfree memory:  %uMiB\n",
		//        physicalmemory::get_total_ram() / 1024 / 1024,
		//        physicalmemory::get_free_ram() / 1024 / 1024);
		//  ps2_handler();
		//  shell_loop();
		for (uint16_t i = 0; i < 10; i++) {
			asm volatile("hlt");
		}
	}
}

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

#if 1

void test() {
	// typeid 0FC63DAF-8483-4772-8E79-3D69D8477DE4
	UUID linux_fs_type = {0x0FC63DAF,
	                      0x8483,
	                      0x4772,
	                      {0x8E, 0x79, 0x3D, 0x69, 0xD8, 0x47, 0x7D, 0xE4}};

	KLIST_FOREACH(&partition_list, part_head) {
		Partition *part = container_of(part_head, Partition, global);
		if (memcmp(&linux_fs_type, &part->type, sizeof(UUID)) == 0) {
			parse_ext(part);
		}
	}
}

#else
void test() {

	if (sizeof(FAT_BPB) != 90) {
		printf("error fat_size: %lu must be 90", sizeof(FAT_BPB));
	}
	if (disk_list.next == &disk_list) {
		printf("no disks registed");
	}
	uint8_t *boot_sector = (uint8_t *)kalloc(512);
	KLIST_FOREACH(&disk_list, head) {
		Disk *disk = container_of(head, Disk, global);
		disk->disk_ops.read(disk, 0, boot_sector, 1);
		MBR *mbr = (MBR *)boot_sector;
	}
	Disk *disk = container_of(disk_list.next->next, Disk, global);
	if (disk->disk_ops.read == nullptr) {
		printf("disk doesn't have read function");
	}

	disk->disk_ops.read(disk, 0, boot_sector, 1);

	MBR mbr;
	memcpy(&mbr, boot_sector, sizeof(MBR));

	MBRPartitionEntry boot_partition_entry = {};
	for (uint32_t i = 0; i < 4; i++) {
		MBRPartitionEntry entry = mbr.partitions[i];
		if ((entry.boot_indicator & 0x80) &&
		    boot_partition_entry.boot_indicator == 0) {
			boot_partition_entry = mbr.partitions[i];
		}
		printf("----------------------------------------------------\n");
		printf("boot indicator: %hhx\nstart_cylinder: %hhu\nstart_head: %hhu\n"
		       "start_sector: %hhu\nos_type: %hhu\nend_cylinder: %hhu\n"
		       "end_head: %hhu\nend_sector: %hhu\nstarting LBA: %u\n"
		       "size in LBA: %u\n",
		       entry.boot_indicator, entry.start_cylinder, entry.start_head,
		       entry.start_sector, entry.os_type, entry.end_cylinder,
		       entry.end_head, entry.end_sector, entry.starting_LBA,
		       entry.size_in_LBA);
	}
	if (mbr.partitions[0].os_type == 0xEE) {
		UNIMPLEMENTED_NAME("GPT disks");
		printf("only MBR disk are supported");
		return;
	}
	printf("----------------------------------------------------\n");
	uint64_t fat_part_start_sector = boot_partition_entry.starting_LBA;
	memset(boot_sector, 0, 512);
	disk->disk_ops.read(disk, fat_part_start_sector, boot_sector, 1);
	printf("bpb data: \n");
	for (uint64_t i = 0; i < 512; i += 8) {
		printf(" 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx\n",
		       boot_sector[i + 0], boot_sector[i + 1], boot_sector[i + 2],
		       boot_sector[i + 3], boot_sector[i + 4], boot_sector[i + 5],
		       boot_sector[i + 6], boot_sector[i + 7]);
	}
	printf("\n");

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

	printf("cluster count: %lu\n", cluster_count);
	if (cluster_count < 4085) {
		printf("FAT12\n");
	} else if (cluster_count < 65525) {
		printf("FAT16\n");
	} else {
		printf("FAT32\n");
	}
	uint64_t fat_start_sector =
		fat_part_start_sector + fat_bpb.reserved_sector_count;

	disk->disk_ops.read(disk, fat_start_sector, boot_sector, 1);
	printf("fat data(part_start + %hu): \n", fat_bpb.reserved_sector_count);
	for (uint64_t i = 0; i < 512; i += 8) {
		printf(" 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx\n",
		       boot_sector[i + 0], boot_sector[i + 1], boot_sector[i + 2],
		       boot_sector[i + 3], boot_sector[i + 4], boot_sector[i + 5],
		       boot_sector[i + 6], boot_sector[i + 7]);
	}
	printf("\n");

	for (uint64_t sector = fat_part_start_sector +
	                       fat_bpb.reserved_sector_count +
	                       (fat_bpb.number_of_FATs * fat_size),
	              i = 0;
	     i < root_directory_sectors; i++, sector++) {
		disk->disk_ops.read(disk, sector, boot_sector, 1);

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
#endif
