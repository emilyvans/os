#include "driver/acpi.hpp"
#include "driver/console.hpp"
#include "driver/keyboard/keycode.hpp"
#include "driver/pci.hpp"
#include "driver/screen.hpp"
#include "limine/limine_requests.hpp"
#include "memory/physical_memory.hpp"
#include "memory/virtual_memory.hpp"
#include "panic.hpp"
#include "utils.hpp"
#include <stdint.h>

void parse_MCFG(ACPISDTHeader *header);

void init_ACPI() {
	uint64_t memory_offset = hhdm_request.response->offset;

	RSDP *rsdp = (RSDP *)((uint64_t)rsdp_request.response->address);

	if (rsdp->Revision != 2) {
		printf("rsdp revision must be 2\n");
		hcf();
	}

	XSDP *xsdp = (XSDP *)(rsdp);

	XSDT *root_xsdt = (XSDT *)(xsdp->XsdtAddress + memory_offset);

	uint64_t entries =
		(root_xsdt->header.Length - sizeof(root_xsdt->header)) / 8;
	for (uint64_t i = 0; i < entries; i++) {
		ACPISDTHeader *header =
			(ACPISDTHeader *)(root_xsdt->pointer_to_other_sdt[i] +
		                      memory_offset);

		const char signature[5] = {0, 0, 0, 0, 0};
		memcpy((void *)signature, (void *)header->Signature, 4);
		// FACP APIC HPET MCFG WAET(could ignore) BGRT
		if (memcmp(header->Signature, "FACP", 4) == 0) {
			printf("%s %u %u ", signature, header->Revision, header->Length);
		} else if (memcmp(header->Signature, "APIC", 4) == 0) {
			printf("%s %u %u\n", signature, header->Revision, header->Length);
			MADT *madt = (MADT *)header;
			(void)madt;
			uint64_t interrupt_controler_array_start =
				(uint64_t)header + sizeof(MADT);
			uint64_t max_address = (uint64_t)header + header->Length;
			uint64_t interrupt_controler_array_entry =
				interrupt_controler_array_start;
			while (interrupt_controler_array_entry < max_address) {
				InteruptController *controller =
					(InteruptController *)interrupt_controler_array_entry;
				printf("interupt type: %x, size: %u\n", controller->type,
				       controller->size);
				interrupt_controler_array_entry += controller->size;
			}
		} else if (memcmp(header->Signature, "HPET", 4) == 0) {
			printf("%s %u %u ", signature, header->Revision, header->Length);
		} else if (memcmp(header->Signature, "BGRT", 4) == 0) {
			printf("%s %u %u ", signature, header->Revision, header->Length);
			// BGRT *bgrt = (BGRT *)header;
			//  display_BGRT(bgrt);
		} else if (memcmp(header->Signature, "MCFG", 4) == 0) {
			printf("%s %u %u\n", signature, header->Revision, header->Length);
			parse_MCFG(header);
			return;
		} else if (memcmp(header->Signature, "WAET", 4) == 0) {
			// ignore vm helper table for windows vmsi
		} else {
			printf("%s %u %u ", signature, header->Revision, header->Length);
		}
	}
}

void display_BGRT(BGRT *bgrt) {
	BitmapHeader *bitmap_ptr =
		(BitmapHeader *)(bgrt->image_address + hhdm_request.response->offset);
	/*printf("x offset: %u\ny offset: %u\ntype: %u\nstatus: %b\naddress: "
	       "%x\n",
	       bgrt->image_x_offset, bgrt->image_y_offset, bgrt->image_type,
	       bgrt->status, (uint64_t)bitmap_ptr);

	printf("%c%c\n", bitmap_ptr->magic[0], bitmap_ptr->magic[1]);
	printf("DIB size: %u\n", bitmap_ptr->info_header.size);*/

	if (bitmap_ptr->info_header.size != 40) {
		printf("Only bitmaps with DIB size of 40 is supported!\n");
		return;
	}

	/*printf("width: %u\nheight: %u\nbpp: %u\ncompress: %u\ncolors: %u\naddress:
	   "
	       "%x\n size: %uBytes\n img addr: %x\n",
	       bitmap_ptr->info_header.width, bitmap_ptr->info_header.height,
	       bitmap_ptr->info_header.bit_per_pixel,
	       bitmap_ptr->info_header.compression_method,
	       bitmap_ptr->info_header.color_count_in_color_palette,
	       (uint64_t)bitmap_ptr, bitmap_ptr->file_size,
	       bitmap_ptr->pixel_byte_offset);*/

	if (bitmap_ptr->info_header.bit_per_pixel != 24) {
		printf("bitmap support is only for 24bits per pixel for now!\n");
		return;
	}

	printf("probe1 ");

	uint8_t *pixels =
		(uint8_t *)(bgrt->image_address + bitmap_ptr->pixel_byte_offset +
	                hhdm_request.response->offset);

	uint64_t row_size = ((bitmap_ptr->info_header.bit_per_pixel *
	                          bitmap_ptr->info_header.width +
	                      31) /
	                     32) *
	                    4;
	printf("probe2 ");

	BitmapDIB info_header = bitmap_ptr->info_header;

	uint64_t image_page_count = ALIGN_UP(
		info_header.width * info_header.height * sizeof(uint32_t), 4096);
	PhysicalAddress physical_address = physicalmemory::kalloc(image_page_count);
	uint32_t *buffer = (uint32_t *)((uint64_t)physical_address +
	                                hhdm_request.response->offset);
	printf("probe3 ");

	for (int32_t y = 0; y < info_header.height; y++) {
		for (int32_t x = 0; x < info_header.width; x++) {
			uint32_t index = y * info_header.width + x;
			uint64_t pixel_index =
				row_size * (info_header.height - y - 1) + (x * 3);
			uint8_t r = pixels[pixel_index + 2];
			uint8_t g = pixels[pixel_index + 1];
			uint8_t b = pixels[pixel_index + 0];
			buffer[index] = r << 16 | g << 8 | b << 0;
		}
	}
	printf("probe4 ");

	image_blit(buffer, bgrt->image_x_offset, bgrt->image_y_offset,
	           info_header.width, info_header.height);
	printf("probe4 ");

	physicalmemory::kfree(physical_address, image_page_count);
	printf("probe5 ");
}

struct MCFG_allocation {
	uint64_t base_address;
	uint16_t PCI_segment;
	uint8_t start_Bus;
	uint8_t end_bus;
	uint32_t reserved;
} __attribute__((packed));

struct pci_header {
	uint16_t Vendor_ID;
	uint16_t device_ID;
	uint16_t command;
	uint16_t status;
	uint8_t revision_ID;
	uint8_t prog_IF;
	uint8_t subclass;
	uint8_t class_code;
	uint8_t cache_line_size;
	uint8_t latency_timer;
	uint8_t header_type;
	uint8_t BIST;
	union {
		struct {
			uint32_t BAR0;
			uint32_t BAR1;
			uint32_t BAR2;
			uint32_t BAR3;
			uint32_t BAR4;
			uint32_t BAR5;
			uint32_t Cardbus_CIS_pointer;
			uint16_t subsystem_vendor_id;
			uint16_t subsystem_id;
			uint32_t expansion_ROM_base_address;
			uint8_t capabilities_pointer;
			uint8_t reserved0[3];
			uint32_t reserved1;
			uint8_t interrupt_line;
			uint8_t interrupt_pin;
			uint8_t min_grant;
			uint8_t max_latency;
		} type_0;
		struct {
			uint32_t BAR0;
			uint32_t BAR1;
			uint8_t primary_bus_number;
			uint8_t secondary_bus_number;
			uint8_t subordinate_bus_number;
			uint8_t secondary_latency_timer;
			uint8_t IO_base;
			uint8_t IO_limit;
			uint16_t secondary_status;
			uint16_t memory_base;
			uint16_t memory_limit;
			uint16_t prefetchable_memory_base;
			uint16_t prefetchable_memory_limit;
			uint32_t prefetchable_base_upper;
			uint32_t prefetchable_limit_upper;
			uint16_t IO_base_upper;
			uint16_t IO_limit_upper;
			uint8_t capability_pointer;
			uint8_t reserved0[3];
			uint32_t expansion_rom_base_address;
			uint8_t interrupt_line;
			uint8_t interrupt_pin;
			uint16_t bridge;
		} type_1;
		struct {
			uint32_t cardbus_socket_base_address;
			uint8_t offset_of_capabilities_list;
			uint8_t reserved0;
			uint16_t secondary_status;
			uint8_t PCI_bus_number;
			uint8_t cardbus_bus_number;
			uint8_t subordinate_bus_number;
			uint8_t cardbus_latency_timer;
			uint32_t memory_base_address0;
			uint32_t memory_limit0;
			uint32_t memory_base_address1;
			uint32_t memory_limit1;
			uint32_t IO_base_address0;
			uint32_t IO_limit0;
			uint32_t IO_base_address1;
			uint32_t IO_limit1;
			uint8_t interrupt_line;
			uint8_t interrupt_pin;
			uint16_t bridge;
			uint16_t subsystem_device_id;
			uint16_t subsystem_vendor_id;
			uint32_t pc_card_legacy_mode_16_bit_base_address;
		} type_2;
	};
} __attribute__((packed));

typedef struct pci_capability {
	uint8_t vendor;
	uint8_t next;
} __attribute__((packed)) pci_capability;

typedef struct virtio_pci_capability {
	pci_capability capability;
	uint8_t length;
	uint8_t config_type;
	uint8_t bar;
	uint8_t id;
	uint8_t padding[2];
	uint32_t offset;
	uint32_t struct_length;
} __attribute__((packed)) virtio_pci_capability;

void print_pci_device(volatile pci_header *pci_device, uint8_t function = 0) {
	if (pci_device->header_type & 0x80) {
		printf("-----------------------------------%u\n", (uint64_t)function);
	} else {
		printf("------------------------------------\n");
	}
	printf("vendor: 0x%x\ndevice: 0x%x\ncommand: 0x%x\nstatus: "
	       "%b\nrevision_id: 0x%x\nprog_if: 0x%x\nclass: 0x%x\n"
	       "subclass: 0x%x\ncacheline_size: 0x%x\nlatency_timer: "
	       "0x%x\ntype: 0x%x\nBIST: 0x%x\n",
	       (uint64_t)pci_device->Vendor_ID, (uint64_t)pci_device->device_ID,
	       (uint64_t)pci_device->command, (uint64_t)pci_device->status,
	       (uint64_t)pci_device->revision_ID, (uint64_t)pci_device->prog_IF,
	       (uint64_t)pci_device->class_code, (uint64_t)pci_device->subclass,
	       (uint64_t)pci_device->cache_line_size,
	       (uint64_t)pci_device->latency_timer,
	       (uint64_t)pci_device->header_type, (uint64_t)pci_device->BIST);

	if ((pci_device->header_type & 0x7F) == 0x0) {
		printf("bar0: 0x%x\nbar1: 0x%x\nbar2: 0x%x\nbar3: 0x%x\nbar4: "
		       "0x%x\nbar5: 0x%x\ncardbus_CIS_pointer: 0x%x\n"
		       "subsystem_vendor: 0x%x\nsubsystem: 0x%x\n"
		       "expansion_rom: 0x%x\ncapabilities_pointer: 0x%x\n"
		       "interrupt_line: 0x%x\ninterrupt_pin: 0x%x\nmin_grant: "
		       "0x%x\nmax_latency: 0x%x\n",
		       (uint64_t)pci_device->type_0.BAR0,
		       (uint64_t)pci_device->type_0.BAR1,
		       (uint64_t)pci_device->type_0.BAR2,
		       (uint64_t)pci_device->type_0.BAR3,
		       (uint64_t)pci_device->type_0.BAR4,
		       (uint64_t)pci_device->type_0.BAR5,
		       (uint64_t)pci_device->type_0.Cardbus_CIS_pointer,
		       (uint64_t)pci_device->type_0.subsystem_vendor_id,
		       (uint64_t)pci_device->type_0.subsystem_id,
		       (uint64_t)pci_device->type_0.expansion_ROM_base_address,
		       (uint64_t)pci_device->type_0.capabilities_pointer,
		       (uint64_t)pci_device->type_0.interrupt_line,
		       (uint64_t)pci_device->type_0.interrupt_pin,
		       (uint64_t)pci_device->type_0.min_grant,
		       (uint64_t)pci_device->type_0.max_latency);
	}
}

typedef struct BarAddress {
	bool is_memory_space;
	uint64_t address;
} BarAddress;

BarAddress get_address_from_bar(volatile pci_header *pci_device,
                                uint8_t bar_index) {
	void *bar_address =
		(void *)&((uint32_t *)&pci_device->type_0.BAR0)[bar_index];
	BarAddress result;
	uint32_t bar = *(uint32_t *)bar_address;
	if ((bar & 1) == 1) { // I/O space
		result.is_memory_space = false;
		result.address = ((uint64_t)bar & ~(0x3));
	} else if (bar & 0x4) { // 64 bit memory address
		result.is_memory_space = true;
		result.address = ((*(uint64_t *)bar_address) & ~(0xF));
	} else { // 32 bit memory address
		result.is_memory_space = true;
		result.address = ((uint64_t)bar & ~(0xF));
	}
	return result;
}

#define VIRTIO_PCI_CAP_COMMON_CFG 1
#define VIRTIO_PCI_CAP_NOTIFY_CFG 2
#define VIRTIO_PCI_CAP_ISR_CFG 3
#define VIRTIO_PCI_CAP_DEVICE_CFG 4

typedef struct virtio_blk_config {
	uint64_t capacity;
	uint32_t size_max;
	uint32_t seg_max;
	struct virtio_blk_geometry {
		uint16_t cylinders;
		uint8_t heads;
		uint8_t sectors;
	} geometry;
	uint32_t blk_size;
	struct virtio_blk_topology {
		// # of logical blocks per physical block (log2)
		uint8_t physical_block_exp;
		// offset of first aligned logical block
		uint8_t alignment_offset;
		// suggested minimum I/O size in blocks
		uint16_t min_io_size;
		// optimal (suggested maximum) I/O size in blocks
		uint32_t opt_io_size;
	} topology;
	uint8_t writeback;
	uint8_t unused0;
	uint16_t num_queues;
	uint32_t max_discard_sectors;
	uint32_t max_discard_seg;
	uint32_t discard_sector_alignment;
	uint32_t max_write_zeroes_sectors;
	uint32_t max_write_zeroes_seg;
	uint8_t write_zeroes_may_unmap;
	uint8_t unused1[3];
	uint32_t max_secure_erase_sectors;
	uint32_t max_secure_erase_seg;
	uint32_t secure_erase_sector_alignment;
	struct virtio_blk_zoned_characteristics {
		uint32_t zone_sectors;
		uint32_t max_open_zones;
		uint32_t max_active_zones;
		uint32_t max_append_sectors;
		uint32_t write_granularity;
		uint8_t model;
		uint8_t unused2[3];
	} zoned;
} __attribute__((packed)) virtio_blk_config;

PCIDevice virtio_blk;

void parse_MCFG(ACPISDTHeader *header) {
	printf("entries: %u\n", ((header->Length - 8 - sizeof(ACPISDTHeader)) /
	                         sizeof(MCFG_allocation)));
	MCFG_allocation *mcfg =
		(MCFG_allocation *)(((uint64_t)header) + 8 + sizeof(ACPISDTHeader));

	uint64_t ecam_virtual_base =
		hhdm_request.response->offset + mcfg->base_address;
	uint64_t ecam_size =
		(mcfg->end_bus - mcfg->start_Bus + 1) * 1024 * 1024; // 1 MB per bus

	for (uint64_t offset = 0; offset < ecam_size; offset += 4096) {
		virtualmemory::map_kernel_page(ecam_virtual_base + offset,  // virtual
		                               mcfg->base_address + offset, // physical
		                               virtualmemory::present_flag |
		                                   virtualmemory::Cache_disable_flag |
		                                   virtualmemory::readwrite_flag);
	}
	volatile pci_header *virtio_block_device = NULL;
	for (uint64_t bus = mcfg->start_Bus; bus <= mcfg->end_bus; bus++) {
		for (uint16_t dev = 0; dev < 32; dev++) {
			volatile pci_header *pci_device =
				(pci_header *)((ecam_virtual_base) +
			                   ((uint64_t(bus) << 20) |
			                    ((uint64_t(dev) & 0x1F) << 15) |
			                    ((uint64_t(0) & 0x7) << 12)));
			if (pci_device->Vendor_ID == 0xFFFF /*|| t->class_code == 0*/)
				continue;
			if (pci_device->Vendor_ID == 0x1AF4 &&
			    pci_device->device_ID == 0x1001) {
				virtio_block_device = pci_device;
				virtio_blk.bus = bus;
				virtio_blk.device_number = dev;
				virtio_blk.function = 0;
				virtio_blk.device_id = pci_device->device_ID;
				virtio_blk.vendor_id = pci_device->Vendor_ID;
				virtio_blk.class_code = pci_device->class_code;
				virtio_blk.subclass = pci_device->subclass;
				virtio_blk.config_address = (uint64_t)pci_device;
			}
			// print_pci_device(pci_device);
			if (pci_device->header_type & 0x80) {
				for (uint8_t i = 1; i < 8; i++) {
					volatile pci_header *pci_device =
						(pci_header *)((ecam_virtual_base) +
					                   ((uint64_t(bus) << 20) |
					                    ((uint64_t(dev) & 0x1F) << 15) |
					                    ((i & 0x7) << 12)));
					if (pci_device->Vendor_ID == 0xFFFF) {
						continue;
					}
					// TODO: create pci device and register it
					// register_pci_device(PCIDevice *device)

					//  print_pci_device(pci_device, i);
				}
			} else {
				// TODO: create pci device and register it
				// register_pci_device(PCIDevice *device)
			}
		}
	}
	if (virtio_block_device == NULL) {
		printf("------------------------------------\n");
		printf("virtio block device not found\n");
		return;
	}
	return;
	print_pci_device(virtio_block_device);
	printf("------------------------------------\n");
	pci_capability *capability =
		(pci_capability *)((uint64_t)virtio_block_device +
	                       (virtio_block_device->type_0.capabilities_pointer &
	                        ~0x3));
	// 64-bit bar 4: bar5[63:32] bar4[31:0]
	while (capability != NULL) {
		if (capability->vendor == 0x9) {
			virtio_pci_capability *virtio_capability =
				(virtio_pci_capability *)capability;
			printf("vendor: 0x%x\nnext: 0x%x\nlength: 0x%x\nconfig_type: "
			       "0x%x\nbar: 0x%x\nid: 0x%x\noffset: 0x%x\nstruct_length: "
			       "0x%x\n",
			       (uint64_t)virtio_capability->capability.vendor,
			       (uint64_t)virtio_capability->capability.next,
			       (uint64_t)virtio_capability->length,
			       (uint64_t)virtio_capability->config_type,
			       (uint64_t)virtio_capability->bar,
			       (uint64_t)virtio_capability->id,
			       (uint64_t)virtio_capability->offset,
			       (uint64_t)virtio_capability->struct_length);

			BarAddress addr = get_address_from_bar(virtio_block_device,
			                                       virtio_capability->bar);

			printf("addr: %x, type: %s\n",
			       addr.address + virtio_capability->offset,
			       addr.is_memory_space ? "memory" : "I/O");

			if (addr.is_memory_space) {

				for (uint64_t offset = 0; offset <= virtio_capability->offset;
				     offset += 4096) {
					virtualmemory::map_kernel_page(
						addr.address + hhdm_request.response->offset +
							offset,            // virtual
						addr.address + offset, // physical
						virtualmemory::present_flag |
							virtualmemory::Cache_disable_flag |
							virtualmemory::readwrite_flag);
				}

				if (virtio_capability->config_type ==
				    VIRTIO_PCI_CAP_DEVICE_CFG) {
					virtio_blk_config *blk_cfg =
						(virtio_blk_config *)(addr.address +
					                          hhdm_request.response->offset +
					                          virtio_capability->offset);

					printf("size: %u\ncylinders: %u\nheads: %u\nsectors: %u\n",
					       blk_cfg->capacity, blk_cfg->geometry.cylinders,
					       blk_cfg->geometry.heads, blk_cfg->geometry.sectors);
				} else if (virtio_capability->config_type ==
				           VIRTIO_PCI_CAP_COMMON_CFG) {
					uint32_t *blk_cfg =
						(uint32_t *)(addr.address +
					                 hhdm_request.response->offset +
					                 virtio_capability->offset);

					printf("feaures: %b\n", (uint64_t)blk_cfg[1]);
				}
			}
		} else {
			printf("vendor: %x\nnext: %x\n", (uint64_t)capability->vendor,
			       (uint64_t)capability->next);
		}
		printf("------------------------------------\n");
		if (capability->next == 0)
			break;

		capability = (pci_capability *)((uint64_t)virtio_block_device +
		                                (capability->next & ~0x3));
	}
}
