#include "driver/virtio_blk.hpp"
#include "driver/console.hpp"
#include "driver/pci.hpp"
#include "limine/limine_requests.hpp"
#include "memory/virtual_memory.hpp"
#include <stdint.h>

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

PCIDeviceID id_table[] = {{.vendor_id = 0x1AF4, .device_id = 0x1001}, {0}};

int virtio_blk_probe(PCIDevice *device) {
	pci_header *virtio_block_device = (pci_header *)device->config_address;
	print_pci_device(virtio_block_device);
	pci_capability *capability =
		(pci_capability *)((uint64_t)virtio_block_device +
	                       (virtio_block_device->type_0.capabilities_pointer &
	                        ~0x3));
	// 64-bit bar 4: bar5[63:32] bar4[31:0]
	bool mapped_bar[] = {false, false, false, false, false};
	while (capability != nullptr) {
		printf("address: 0x%x\n", capability);
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

			printf("addr: 0x%x, type: %s, size: 0x%x\n",
			       addr.address + virtio_capability->offset,
			       addr.is_memory_space ? "memory" : "I/O", addr.size);

			if (addr.is_memory_space) {
				if (!mapped_bar[virtio_capability->bar]) {
					for (uint64_t offset = 0; offset < addr.size;
					     offset += 4096) {
						uint64_t phys = addr.address + offset;
						uint64_t virt = phys + hhdm_request.response->offset;
						printf("virtual: 0x%x, physical: 0x%x\n", virt, phys);

						virtualmemory::map_kernel_page(
							virt, phys,
							virtualmemory::present_flag |
								virtualmemory::Cache_disable_flag |
								virtualmemory::readwrite_flag);
					}
					mapped_bar[virtio_capability->bar] = true;
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
	return 0;
};

PCIDriver virtio_blk_drv = {
	.name = "virtio_blk",
	.id_table = id_table,
	.probe = virtio_blk_probe,
};
